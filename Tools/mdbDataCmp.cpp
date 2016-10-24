/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbDataCmp.h     
*@Description���Ƚ�����һ̨�����߶�̨���Զ������뱾���Ա������Ƿ���ͬ
*@Author:       jiang.lili
*@Date��        2014��4��
*@History:
******************************************************************************************/
#include <dirent.h>
#include "Tools/mdbDataCmp.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbErr.h"
#include "Helper/mdbFileList.h"
#include "Helper/mdbOS.h"

//namespace QuickMDB{


    void TMdbDataCheckDSN::Clear()
    {
        m_eType = E_DSN_UNKOWN;
        m_strDsn.clear();
        m_strUser.clear();
        m_strPassword.clear();
        m_strFilterSql.clear();
        m_strIP.clear();
        m_iPort = -1;
        m_bAll = true;
        m_vQdgTable.clear();
        m_vTables.clear();
    }

    void TMdbDataCheckInfo::Clear()
    {
        m_strDsn.clear();

        m_iCmpTimes = 3;
        m_iThreadNum = 1;
        m_iLogLevel = 0;
    }

    void TMdbDataCheckTable::Clear()
    {
        m_iPkLen = 0;
        m_iRecordNum = 0;
        m_strTableName.clear();
        m_iTableID = -1;
        m_bDealed = false;
        m_bIsQdgTable = false;
        m_strLoadSql.clear();
    }

    TMdbDataCheckConfig::TMdbDataCheckConfig()
    {

    }

    TMdbDataCheckConfig::~TMdbDataCheckConfig()
    {

    }

    int TMdbDataCheckConfig::Init(std::string strCfgFile, std::string strDataSource)
    {
        int iRet = 0;
        m_tCheckInfo.Clear();
        m_tDsn.Clear();
        m_sDataSource = strDataSource;//���Ƚ�����Դ����

        //��ȡ�����ļ�
        MDBXMLDocument tDoc(strCfgFile);
        if (false == tDoc.LoadFile())
        {
            TADD_ERROR(ERROR_UNKNOWN,"LoadXMLFile(name=%s) failed.", strCfgFile.c_str());
            return -1;
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBCheckConfig");
        if(NULL == pRoot)
        {
            TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"MDBConfig node does not exist in the file[%s].", strCfgFile.c_str());
            return ERR_APP_CONFIG_ITEM_NOT_EXIST;
        }

        CHECK_RET(LoadSys(pRoot), "LoadSys failed.");
        CHECK_RET(LoadDsn(pRoot), "LoadDsn failed.");
        return iRet;
    }
    int TMdbDataCheckConfig::LoadSys(MDBXMLElement* pRoot)
    {
        int iRet = 0;
        MDBXMLElement* pESys = pRoot->FirstChildElement("sys");
        CHECK_OBJ(pESys);

        MDBXMLElement* pESec = NULL;
        MDBXMLAttribute* pAttr = NULL;

        pESec = pESys->FirstChildElement("dsn");
        CHECK_OBJ(pESec);
        pAttr  = pESec->FirstAttribute();
        CHECK_OBJ(pAttr);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0 )
        {
            m_tCheckInfo.m_strDsn.assign(pAttr->Value());
        }

        pESec = pESys->FirstChildElement("compare_times");
        CHECK_OBJ(pESec);
        pAttr  = pESec->FirstAttribute();
        CHECK_OBJ(pAttr);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0 )
        {
            m_tCheckInfo.m_iCmpTimes = atoi(pAttr->Value());
        }

        pESec = pESys->FirstChildElement("thread_count");
        CHECK_OBJ(pESec);
        pAttr  = pESec->FirstAttribute();
        CHECK_OBJ(pAttr);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0 )
        {
            m_tCheckInfo.m_iThreadNum = atoi(pAttr->Value());
        }

        pESec = pESys->FirstChildElement("check_interval");
        CHECK_OBJ(pESec);
        pAttr  = pESec->FirstAttribute();
        CHECK_OBJ(pAttr);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0 )
        {
            m_tCheckInfo.m_iInterval = atoi(pAttr->Value());
        }

        pESec = pESys->FirstChildElement("log_level");
        CHECK_OBJ(pESec);
        pAttr  = pESec->FirstAttribute();
        CHECK_OBJ(pAttr);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0 )
        {
            m_tCheckInfo.m_iLogLevel = atoi(pAttr->Value());
        }

        pESec = pESys->FirstChildElement("file_path");
        CHECK_OBJ(pESec);
        pAttr  = pESec->FirstAttribute();
        CHECK_OBJ(pAttr);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0 )
        {
            m_tCheckInfo.m_strPath.assign(pAttr->Value());
        }

        return iRet;
    }

    int TMdbDataCheckConfig::LoadDsn(MDBXMLElement* pRoot)
    {
        int iRet = 0;

        MDBXMLElement* pEDbSec = NULL;
        MDBXMLAttribute* pAttr = NULL;
        bool bFound = false;

        for (MDBXMLElement* pSec=pRoot->FirstChildElement("DataSource"); pSec; pSec=pSec->NextSiblingElement("DataSource"))
        {
            pEDbSec = pSec->FirstChildElement("db");
            CHECK_OBJ(pEDbSec);
            pAttr = pEDbSec->FirstAttribute();
            CHECK_OBJ(pAttr);
            if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"name")!=0)//db�ε�һ�����Ա���Ϊ��name��
            {
                CHECK_RET(-1, "xml format error.");
            }
            else
            {
                if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), m_sDataSource.c_str()) != 0)//����Ҫ�Ƚϵ�����Դ
                {
                    continue;
                }
                else
                {
                    bFound = true;    
                    CHECK_RET(LoadDbInfo(pAttr), "LoadDbInfo() failed.");

                    pEDbSec = pSec->FirstChildElement("table_list");
                    if (pEDbSec!=NULL)
                    {
                        pAttr = pEDbSec->FirstAttribute();
                        CHECK_OBJ(pAttr);
                        if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"value") == 0 && TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "all") == 0)
                        {
                            m_tDsn.m_bAll = true;
                        TADD_NORMAL("Check all the tables in dsn[%s]", m_tDsn.m_strDsn.c_str());
                        }
                        else
                        {
                            m_tDsn.m_bAll = false;
                            TMdbNtcSplit tSplit;
                            tSplit.SplitString(pAttr->Value(), ",");//���ŷָ�����
                            char sTmp[MAX_NAME_LEN];
                            sTmp[0] = '\0';
                            for (unsigned int i = 0; i < tSplit.GetFieldCount(); i++)
                            {
                                SAFESTRCPY(sTmp, MAX_NAME_LEN,tSplit[i]);
                                TMdbNtcStrFunc::Trim(sTmp);
                                m_tDsn.m_vTables.push_back(sTmp);
                            }
                        }
                    }

                    pEDbSec = pSec->FirstChildElement("filter_sql");
                    if (pEDbSec!=NULL)
                    {
                        pAttr = pEDbSec->FirstAttribute();
                        CHECK_OBJ(pAttr);
                        if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"value") == 0 &&  strlen(pAttr->Value()) !=0)
                        {
                            m_tDsn.m_strFilterSql = pAttr->Value();
                        }
                    }

                    if (m_tDsn.m_eType == E_DSN_ORACLE)//��mdb���бȽϣ�����Ҫ����qdg��
                    {
                        CHECK_RET(LoadQdgTable(pSec), "LoadQdgTable() failed.");
                    }
                    
                    break;
                }
            }
        }
        if (!bFound)
        {
            CHECK_RET(-1, "Input db name [%s] not found in configuration file.", m_sDataSource.c_str());
        }
        if (m_tDsn.m_strFilterSql.size()> 0 && (m_tDsn.m_bAll == true || m_tDsn.m_vTables.size()>1))
        {
            TADD_WARNING("Make sure the filter sql is for all the table to compare.");
        }
        return iRet;
    }

    int TMdbDataCheckConfig::LoadDbInfo(MDBXMLAttribute* pAttr)
    {
        int iRet = 0;
        for(pAttr=pAttr->Next(); pAttr; pAttr=pAttr->Next())
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "type") == 0)
            {
                if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(),"oracle")==0)
                {
                    m_tDsn.m_eType = E_DSN_ORACLE;
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "mdb") == 0)
                {
                    m_tDsn.m_eType = E_DSN_MDB;
                }
                else
                {
                    CHECK_RET(-1, "Unknown dsn type");
                }
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "dsn") == 0)
            {
                m_tDsn.m_strDsn = pAttr->Value();
            }
            else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "user") == 0)
            {
                m_tDsn.m_strUser = pAttr->Value();
            }
            else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "password") == 0)
            {
                m_tDsn.m_strPassword = pAttr->Value();
            }
            else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ip") == 0)
            {
                m_tDsn.m_strIP = pAttr->Value();
            }
            else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "port") == 0)
            {
                m_tDsn.m_iPort = atoi(pAttr->Value());
            }
        }
        return iRet;
    }

    int TMdbDataCheckConfig::LoadQdgTable(MDBXMLElement* pESec)
    {
        int iRet = 0;
        MDBXMLElement* pQdgSec = pESec->FirstChildElement("qdg_table");
        MDBXMLAttribute* pAttr = NULL;
        TMdbCheckQdgTable tQdgTable;
        for (; pQdgSec !=NULL; pQdgSec= pQdgSec->NextSiblingElement("qdg_table"))
        {
            tQdgTable.Clear();
            pAttr = pQdgSec->FirstAttribute();
            for (; pAttr!=NULL; pAttr= pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {     
                    if (!m_tDsn.m_bAll && !CheckQdgTable(pAttr->Value()))
                    {
                        CHECK_RET(-1, "Invalid qdg table name[%s], not included in 'table_list'", pAttr->Value());
                    }
                    tQdgTable.m_strTableName = pAttr->Value();
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "Load_sql") == 0)
                {
                    if (strlen(pAttr->Value())== 0)
                    {
                        CHECK_RET(-1, "QDG table[%s]'s load_sql is empty.", tQdgTable.m_strTableName.c_str());
                    }
                    tQdgTable.m_strLoadSql = pAttr->Value();
                }   
            }
            if (tQdgTable.m_strLoadSql.size() == 0 || tQdgTable.m_strTableName.size() == 0)
            {
                CHECK_RET(-1, "Invalid QDG table, Name = [%s] , LoadSQL = [%s]", tQdgTable.m_strTableName.c_str(), tQdgTable.m_strLoadSql.c_str());
            }
            m_tDsn.m_vQdgTable.push_back(tQdgTable);
        }


        return iRet;
    }

    bool TMdbDataCheckConfig::CheckQdgTable(std::string strTable)
    {
        bool bFound = false;
        for (long unsigned int i = 0 ; i< m_tDsn.m_vTables.size(); i++)
        {
            if (TMdbNtcStrFunc::StrNoCaseCmp(m_tDsn.m_vTables[i].c_str(), strTable.c_str()) == 0)
            {
                bFound = true;
                break;
            }
        }
        return bFound;
    }

    TMdbDiffFile::TMdbDiffFile(): m_pFile(NULL), m_iBufLen(0), m_pTmpBuf(NULL)
    {
    }

    TMdbDiffFile::~TMdbDiffFile()
    {
        fclose(m_pFile);
        m_pFile = NULL;
        SAFE_DELETE_ARRAY(m_pTmpBuf);
    }

    int TMdbDiffFile::Init(std::string strPath, std::string strTable, int iBufLen)
    {
        m_iBufLen = iBufLen;
        m_pTmpBuf = new(std::nothrow) char[m_iBufLen];
        CHECK_OBJ(m_pTmpBuf);

        strPath.append("DIFF_");
        strPath.append(strTable.c_str());
        m_pFile = fopen(strPath.c_str(), "w+");
        return m_pFile==NULL? -1: 0;
    }

    int TMdbDiffFile::Open(std::string strPath, std::string strTable,  int iBufLen)
    {
        int iRet = 0;
        m_iBufLen = iBufLen;
        if (NULL == m_pTmpBuf)
        {
            m_pTmpBuf = new(std::nothrow) char[m_iBufLen];
            CHECK_OBJ(m_pTmpBuf);
        }

        strPath.append("DIFF_");
        strPath.append(strTable.c_str());
        m_pFile = fopen(strPath.c_str(), "r+");
        CHECK_RET(fseek(m_pFile, 0, SEEK_SET), "fseek failed, error = %d", errno);

        return m_pFile==NULL? -1: 0;
    }

    int TMdbDiffFile::WriteDiffRcd(TMDBDiffRcd *pDiffRcd)
    {
        fprintf(m_pFile, "%c:%c %s\n", pDiffRcd->cDiffType, pDiffRcd->cRecheckSame, pDiffRcd->strPk.c_str());
        return 0;
    }

    int TMdbDiffFile::GetNextDiffRcd(TMDBDiffRcd* pDiffRcd)
    {
        m_pTmpBuf[0] = '\0';
        if (fgets(m_pTmpBuf, m_iBufLen, m_pFile) == NULL)//�����ļ�ĩβ
        {
            return false;
        }

        //TADD_NORMAL("DiffRcd : %s", m_pTmpBuf);

        pDiffRcd->cDiffType = m_pTmpBuf[0];
        pDiffRcd->cRecheckSame = m_pTmpBuf[2];
        pDiffRcd->strPk.assign(&m_pTmpBuf[4]);
        return true;
    }

    int TMdbDiffFile::UpdateDiffRcdSame()
    {
        int iRet = 0;
        CHECK_RET(fseek(m_pFile, 0-(long int)(strlen(m_pTmpBuf))+2, SEEK_CUR),"fseek failed, error = %d", errno);
        iRet = fputc('Y', m_pFile);
        CHECK_RET(fseek(m_pFile, (long int)(strlen(m_pTmpBuf)) -3, SEEK_CUR), "fseek failed, error = %d", errno);
        return iRet == 'Y' ? 0 : -1;
    }

    int TMdbDiffFile::UpdateDiffType(int iDiffType)
    {
        int iRet = 0;
        CHECK_RET(fseek(m_pFile, 0-(long int)(strlen(m_pTmpBuf)), SEEK_CUR), "fseek failed, error = %d", errno);
        iRet = fputc(iDiffType, m_pFile);
        CHECK_RET(fseek(m_pFile, (long int)(strlen(m_pTmpBuf)) -1, SEEK_CUR), "fseek failed, error = %d", errno);
        return iRet == iDiffType ? 0 : -1;

    }

    TMdbHashFile::TMdbHashFile():m_pFile(NULL)
    {

    }

    TMdbHashFile::~TMdbHashFile()
    {
        fclose(m_pFile);
        m_pFile = NULL;
        SAFE_DELETE_ARRAY(m_pPkBuf);
    }

    const int TMdbHashFile::m_iDefaultHash = 997;

    int TMdbHashFile::Init(std::string strPath, std::string strTable, int iRcdNum, int iPkLen)
    {
        int iRet = 0;
        strPath.append("PK_");
        strPath.append(strTable);
        m_pFile = fopen(strPath.c_str(), "w+");
        CHECK_OBJ(m_pFile);

        if(iRcdNum <= m_iDefaultHash)
        {
            m_iRcdNum = m_iDefaultHash;
        }
        else
        {
            m_iRcdNum = iRcdNum;
        }
        m_iPkLen = iPkLen;
        m_lCurMaxPos = 0;
        long  iOffset = 0;

        memset(m_sPosBuf,0,MDB_DATA_CHECK_MAX_POS_LEN);
        m_sPosBuf[0]='-';
        m_sPosBuf[1] ='1';

        m_pPkBuf = new(std::nothrow) char[m_iPkLen+1];
		if(m_pPkBuf == NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new m_pPkBuf");
			return ERR_OS_NO_MEMROY;
		}
        memset(m_pPkBuf, 0, (size_t)(m_iPkLen+1));
        m_pPkBuf[iPkLen] ='\n';

        //��ʼ����������hashֵ����һ����¼λ��Ϊ-1
        for (; m_lCurMaxPos < m_iRcdNum; m_lCurMaxPos++)
        {
            iOffset = (long)(m_lCurMaxPos * (MDB_DATA_CHECK_MAX_POS_LEN + m_iPkLen+1));

            CHECK_RET(fseek(m_pFile, iOffset, SEEK_SET), "fseek failed, error = %d", errno);

            if (fwrite(m_sPosBuf, MDB_DATA_CHECK_MAX_POS_LEN, 1, m_pFile) !=1)//��¼λ��
            {
                CHECK_RET(-1, "Write pk to file error, errno = %d", errno);
            }

            if (fwrite(m_pPkBuf, (size_t)(iPkLen+1), 1, m_pFile) !=1)//��¼λ��
            {
                CHECK_RET(-1, "Write pk to file error, errno = %d", errno);
            }
        }


        return iRet;
    }

    int TMdbHashFile::GetHashValue(std::string strValue)
    {
        int llValue = 0;
        llValue = (int)(TMdbNtcStrFunc::StrToHash(strValue.c_str()));
        llValue = llValue%m_iRcdNum;
        
        llValue = llValue<0? -llValue:llValue;
        return llValue;
    }

    int TMdbHashFile::WritePk(std::string strPK)
    {
        int iRet = 0;
        long long  llValue = GetHashValue(strPK);
        long iOffSet = -1;

        //������һ������д��λ��
        while(1)
        {
            iOffSet =  (long)(llValue*(MDB_DATA_CHECK_MAX_POS_LEN + m_iPkLen+1));
            CHECK_RET(fseek(m_pFile, iOffSet, SEEK_SET), "fseek failed, error = %d", errno);

            if (fread(m_sPosBuf, MDB_DATA_CHECK_MAX_POS_LEN, 1, m_pFile) == 1)//��ȡλ����Ϣ
            {
                if (m_sPosBuf[0] == '-' && m_sPosBuf[1]== '1')
                {
                    break;
                }
                else
                {
                    llValue = atoi(m_sPosBuf);
                }
            }
            else
            {
                CHECK_RET(-1, "Read file error, errno = %d", errno);
            }
        }
        //д����һ����ͬ�Ĺ�ϣֵ����λ�úͱ�������ֵ
        CHECK_RET(fseek(m_pFile, iOffSet, SEEK_SET), "fseek failed, error = %d", errno);
        fprintf(m_pFile, "%lld", m_lCurMaxPos);
        CHECK_RET(fseek(m_pFile, iOffSet+MDB_DATA_CHECK_MAX_POS_LEN, SEEK_SET), "fseek failed, error = %d", errno);
        fprintf(m_pFile, "%s", strPK.c_str());

        //����һ������λ����д-1
        iOffSet = static_cast<long>(m_lCurMaxPos*(MDB_DATA_CHECK_MAX_POS_LEN + m_iPkLen+1));
        CHECK_RET(fseek(m_pFile, iOffSet, SEEK_SET), "fseek failed, error = %d", errno);
        if (fwrite(m_sPosBuf, MDB_DATA_CHECK_MAX_POS_LEN, 1, m_pFile) !=1)//��¼����ֵ
        {
            CHECK_RET(-1, "Write pos to file error, errno = %d", errno);
        }

        if (fwrite(m_pPkBuf, (size_t)(m_iPkLen+1), 1, m_pFile) !=1)//��¼λ��
        {
            CHECK_RET(-1, "Write pk to file error, errno = %d", errno);
        }

        m_lCurMaxPos++;
        return iRet;
    }

    bool TMdbHashFile::IsPkExist(std::string strPK)
    {
        bool bRet = false;
        //int iRet = 0;
        long long  llValue = GetHashValue(strPK);
        long  iOffSet = -1;
        char* sValue = new(std::nothrow) char[m_iPkLen];
		if(sValue == NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create sValue");
			return false;

		}
        memset(sValue, 0, (size_t)(m_iPkLen));

        while(1)
        {
            iOffSet =  (long)(llValue*(MDB_DATA_CHECK_MAX_POS_LEN + m_iPkLen +1));
            if (fseek(m_pFile, iOffSet, SEEK_SET)!=0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"fseek failed, error = %d", errno);
                return false;
            }

            if (fread(&m_sPosBuf, MDB_DATA_CHECK_MAX_POS_LEN, 1, m_pFile) == 1)//��ȡλ����Ϣ
            {
                llValue = atoi(m_sPosBuf);
                if (llValue == -1)
                {
                    break;//δ�ҵ�
                }
                else
                {
                    if (fread(sValue, (size_t)m_iPkLen, 1, m_pFile) == 1)
                    {
                        sValue[m_iPkLen-1] = '\0';
                        if (strPK.compare(sValue)==0)//�ҵ���ͬ������
                        {
                            bRet = true;
                            break;
                        }
                    }
                    else
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"Read file error, errno = %d", errno);
                        return false;
                    }        
                }
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"Read file error, errno = %d", errno);
                return false;
            }
        }
        SAFE_DELETE(sValue);
        return bRet;
    }



    TMdbCheckThread::TMdbCheckThread()
    {
        m_pDiffFile = NULL;
        m_pPkFile = NULL;

        m_pLocalMdb = NULL;
        m_pOraDB = NULL;
        m_pPeerMdb = NULL;

        m_pLocalQuery = NULL;
        m_pOraQuery = NULL;
        m_pPeerQuery = NULL;

        m_sDeleteSQL[0] = '\0';
        m_sInsertSQL[0] = '\0';
        m_sMdbSelectAllSQL[0] = '\0';
        m_sUpdateSQL[0] = '\0';
        m_sOraSelectByPkSQL[0] = '\0';

        ClearCmpResult();
    }

    TMdbCheckThread::~TMdbCheckThread()
    {
        SAFE_DELETE(m_pDiffFile);
        SAFE_DELETE(m_pPkFile);

        SAFE_DELETE(m_pLocalQuery);
        SAFE_DELETE(m_pOraQuery);
        SAFE_DELETE(m_pPeerQuery);

        SAFE_DELETE(m_pLocalMdb);
        SAFE_DELETE(m_pOraDB);
        SAFE_DELETE(m_pPeerMdb);
    }

    int TMdbCheckThread::Init(TMdbDataCheckConfig *pCheckConfig, TMdbConfig *pMdbConfig, std::string strPath)
    {
        int iRet = 0;
        CHECK_OBJ(pCheckConfig);
        CHECK_OBJ(pMdbConfig);
        m_eState = E_CHECK_THREAD_FREE;
        m_pCheckConfig = pCheckConfig;
        m_pMdbConfig = pMdbConfig;
        m_bExit = false;
        m_strPath = strPath;

        CHECK_RET(ConnectDB(), "ConnectDB() failed.");

        return iRet;
    }

    int TMdbCheckThread::Start()
    {
        int iRet = 0;
        TADD_NORMAL_TO_CLI(FMT_CLI_START, "THREAD [ %d ]", m_iThreadNO);

        SetThreadInfo(this,1024*1024*50);
        CHECK_RET(Run(NULL), "Run TMdbHostCmpThread failed.");

        return iRet;
    }

    void TMdbCheckThread::Stop()
    {
        m_bExit = true;
    }
    int TMdbCheckThread::SetTableToCmp(TMdbDataCheckTable * pTableToCmp)
    {
        int iRet = 0;
        CHECK_OBJ(pTableToCmp);
        if (m_pTableToCmp != NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Thread is not free.");
        }   
        m_pTableToCmp = pTableToCmp;

        TADD_NORMAL("Table [%s] to Compare.", m_pTableToCmp->m_strTableName.c_str());
        m_eState = E_CHECK_THREAD_BUSY;
        return iRet;
    }

    int TMdbCheckThread::SetTableToRestore(TMdbDataCheckTable * pTableToRestore)
    {
        int iRet = 0;
        CHECK_OBJ(pTableToRestore);
        if (m_pTableToRestore != NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Thread is not free.");
        }   
        m_pTableToRestore = pTableToRestore;

        TADD_NORMAL("Table [%s] to Restore.", pTableToRestore->m_strTableName.c_str());
        m_eState = E_CHECK_THREAD_BUSY;
        return iRet;
    }

    int TMdbCheckThread::svc()
    {
        int iRet = 0;
        
        while(!m_bExit)
        {
            if (m_pTableToCmp != NULL)//�Ƚ�һ�ű�
            {
                iRet = CompareOneTable();
                if (iRet < 0)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Compare table[%s] failed.", m_pTableToCmp->m_strTableName.c_str());
                    m_eState = E_CHECK_THREAD_ERROR;
                }
                ClearCmpResult();
            }
            else if(m_pTableToRestore!=NULL)//�ָ�һ�ű�
            {
                iRet = RestoreOneTable();
                if (iRet < 0)
                {
                    m_eState = E_CHECK_THREAD_ERROR;
                    TADD_ERROR(ERROR_UNKNOWN,"Restore table[%s] failed.", m_pTableToRestore->m_strTableName.c_str());
                }
                ClearCmpResult();
            }
            else
            {
                TMdbDateTime::MSleep(10);
            }
        }
        m_eState = E_CHECK_THREAD_OVER;
        return iRet;
    }

    int TMdbCheckThread::ConnectDB()
    {
        int iRet = 0;
        try
        {
            m_pLocalMdb = new(std::nothrow) TMdbDatabase();
            CHECK_OBJ(m_pLocalMdb);
            if (!m_pLocalMdb->ConnectAsMgr(m_pCheckConfig->m_tCheckInfo.m_strDsn.c_str()))
            {
                CHECK_RET(-1, "Connect to DSN[%s] failed.", m_pCheckConfig->m_tCheckInfo.m_strDsn.c_str());
            }
            m_pLocalQuery = new(std::nothrow) TMdbQuery(m_pLocalMdb, 0);
            CHECK_OBJ(m_pLocalQuery);

            if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_ORACLE)
            {
                m_pOraDB = TMDBDBFactory::CeatDB();
                CHECK_OBJ(m_pOraDB);
                if (!m_pOraDB->Connect(m_pCheckConfig->m_tDsn.m_strUser.c_str(), m_pCheckConfig->m_tDsn.m_strPassword.c_str(), m_pCheckConfig->m_tDsn.m_strDsn.c_str()))
                {
                    CHECK_RET(-1, "Connect to Oracle DSN[%s] failed.", m_pCheckConfig->m_tDsn.m_strDsn.c_str());
                }
                m_pOraQuery = m_pOraDB->CreateDBQuery();
                CHECK_OBJ(m_pOraQuery);
            }
            else //mdb
            {
                m_pPeerMdb = new(std::nothrow) TMdbClientDatabase();
                CHECK_OBJ(m_pPeerMdb);
				//ʹ��ocpЭ��,������ǰ��
				m_pPeerMdb->UseOcp();
                m_pPeerMdb->SetServer(m_pCheckConfig->m_tDsn.m_strIP.c_str(), m_pCheckConfig->m_tDsn.m_iPort);
                if (!m_pPeerMdb->Connect(m_pCheckConfig->m_tDsn.m_strUser.c_str(),m_pCheckConfig->m_tDsn.m_strPassword.c_str(), m_pCheckConfig->m_tDsn.m_strDsn.c_str()))
                {
                    CHECK_RET(-1, "Connect to peer mdb DSN[%s] failed.", m_pCheckConfig->m_tDsn.m_strDsn.c_str());
                }
                m_pPeerQuery = m_pPeerMdb->CreateDBQuery();
                CHECK_OBJ(m_pPeerMdb);
            }

        }

        CATCH_MDB_DATA_CHECK_EXEPTION
            return iRet;
    }

    int TMdbCheckThread::CompareOneTable()
    {
        int iRet = 0;
        TADD_NORMAL_TO_CLI(FMT_CLI_START, "THREAD [ %d ] TABLE [%s]: ", m_iThreadNO, m_pTableToCmp->m_strTableName.c_str());
        m_pTable = m_pMdbConfig->GetTableByName(m_pTableToCmp->m_strTableName.c_str());

        //(1) �����ļ�ָ���ʼ��
        m_pPkFile = new(std::nothrow) TMdbHashFile();
        CHECK_OBJ(m_pPkFile);
        CHECK_RET(m_pPkFile->Init(m_strPath, m_pTableToCmp->m_strTableName, m_pTableToCmp->m_iRecordNum, m_pTableToCmp->m_iPkLen+MDB_DATA_CHECK_INT_BUF), "TMdbHashFile init failed.");

        //(2) ���ñȽϵ�SQL
        CHECK_RET(SetSelectAllSQL(), "SetSelectAllSQL() failed.");
        CHECK_RET(SetSelectByPKSQL(), "SetSelectByPKSQL failed.");

        //(3) ���Զ������뱾�����ݽ��бȽϣ�������Զ�����������������ͬ��¼
        if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_MDB)
        {
            CHECK_RET(CmpMdbTable(), "CmpMdbTable() failed.");
        }
        else
        {
            CHECK_RET(CmpOraTable(), "CmpOraTable() failed.");
        }
        TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %d ] records different with Peer.", m_iThreadNO, m_pTable->sTableName,  m_iDiffRcd);

        //(4) ���������ļ�����鱾���Ƿ��������¼�����У����¼��ͬ����
        CHECK_RET(CheckLocalData(), "CheckLocalData failed.");

        //(5) �Բ�ͬ�����ظ��Ƚ�
        CHECK_RET(ReCompare(), "ReCompare failed.");

        TADD_NORMAL_TO_CLI(FMT_CLI_OK, "THREAD [ %d ] TABLE [%s]: Finished.", m_iThreadNO, m_pTableToCmp->m_strTableName.c_str());

        return iRet;
    }

    int TMdbCheckThread::RestoreOneTable()
    {
        int iRet = 0;
        TADD_NORMAL_TO_CLI(FMT_CLI_START, "THREAD [ %d ]: Table[%s]", m_iThreadNO, m_pTableToRestore->m_strTableName.c_str());
        //(1)��ʼ������Ϣ
        m_pTable = m_pMdbConfig->GetTableByName(m_pTableToRestore->m_strTableName.c_str());

    CHECK_RET(SetSelectByPKSQL(), "SetSelectByPKSQL failed.");
        CHECK_RET(SetUpdateSQL(), "SetRestoreUpdateSQL failed.");
        CHECK_RET(SetInsertSQL(), "SetRestoreInsertSQL failed.");
        CHECK_RET(SetDeleteSQL(), "SetRestoreDeleteSQL failed.");

        //(2) ��ʼ���ļ�ָ��
        m_pDiffFile = new(std::nothrow) TMdbDiffFile();
        CHECK_OBJ(m_pDiffFile);
        
        CHECK_RET(m_pDiffFile->Open(m_strPath, m_pTableToRestore->m_strTableName, m_pTableToRestore->m_iPkLen+MDB_DATA_CHECK_INT_BUF), "TMdbDiffFile open failed.");

        //(3)ѡ��ָ�����
        if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_MDB 
            /*&&(m_pTable->tColumn[0].iRepAttr == Column_To_Rep || (m_pTable->tColumn[0].iRepAttr == Column_No_Rep&&!m_pTableToRestore->m_bIsQdgTable) || m_pTable->tColumn[0].iRepAttr ==Column_Ora_Rep)*/)
        {
            //�ԶԶ�Ϊ���ݻָ�,�ָ����еı�
            CHECK_RET(RestoreDataFromPeer(), "RestoreDataFromPeer failed.");
        }
        else if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_ORACLE && (m_pTable->iRepAttr == REP_FROM_DB || m_pTableToRestore->m_bIsQdgTable))
        {
            //��oracleΪ���ݻָ����ָ���oracleͬ���ı�����qdg�ı�
            CHECK_RET(RestoreDataFromOra(), "RestoreDataFromOra failed.");
        }
        else if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_ORACLE
            &&(m_pTable->iRepAttr == REP_TO_DB))
        {
            //�Ա���Ϊ���ݻָ�oracle���ݣ� �ָ���oracleͬ���ı�
            CHECK_RET(RestoreDataToOra(), "RestoreDataToOra failed.");
        }

        TADD_NORMAL_TO_CLI(FMT_CLI_OK, "THREAD [ %d ]: Restore Table[%s]", m_iThreadNO, m_pTableToRestore->m_strTableName.c_str());
        return iRet;
    }

    int TMdbCheckThread::CmpMdbTable()
    {
        int iRet = 0;

        try
        {
            //�Զ˻��������������ļ�¼������������������������ȡ��¼
            m_pPeerQuery->Close();
            m_pPeerQuery->SetSQL(m_sMdbSelectAllSQL);
            m_pLocalQuery->Close();
            m_pLocalQuery->SetSQL(m_sMdbSelectByPkSQL);
            m_pPeerQuery->Open();
            bool bSame = false;
            int iCheckedCount = 0;
            while(m_pPeerQuery->Next())
            {
                CHECK_RET(SetLocalKeyParam(m_pPeerQuery, m_pLocalQuery), "SetLocalKeyParam() failed.");//���ñ���������SQL����������ֵ��
                m_pLocalQuery->Open();//������������������ȡһ����¼
                if (m_pLocalQuery->Next())
                {
                    CHECK_RET(CompareOneMdbRcd(m_pPeerQuery, m_pLocalQuery, bSame), "CompareOneRcd() failed.");//�Ƚ�������¼�Ƿ���ͬ
                    if (!bSame)
                    {
                        CHECK_RET(SaveRecord(m_pPeerQuery, MDB_DIFF_DATA_NOT_SAME), "SaveRecord failed.");//����Զ˻�����
                        //m_iDiffRcd ++; 
                    }
                    else
                    {
                        CHECK_RET(SaveRecord(m_pPeerQuery, MDB_DIFF_DATA_SAME), "SaveRecord failed.");//����Զ˻�����
                    }
                }
                else
                {
                    CHECK_RET(SaveRecord(m_pPeerQuery, MDB_DIFF_LOCAL_NOT_EXIST), "RecordDiffRcd() failed.");//��¼��ͬ����
                    //m_iDiffRcd++;
                }
                iCheckedCount++;
                if (iCheckedCount%10000 == 0)
                {
                    TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %d ] records compared.", m_iThreadNO, m_pTable->sTableName, iCheckedCount);
                }
            }
             TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %d ] records compared.", m_iThreadNO, m_pTable->sTableName, iCheckedCount);
        }
        CATCH_MDB_DATA_CHECK_EXEPTION
            return iRet;
    }

    int TMdbCheckThread::CmpOraTable()
    {
        int iRet = 0;
        try
        {
            //�Զ˻��������������ļ�¼������������������������ȡ��¼
            m_pOraQuery->Close();
            m_pOraQuery->SetSQL(m_sOraSelectAllSQL);
            SetCompexParam(m_pOraQuery);//���ô�oracle ���м���ʱ��Load sql�еĲ���

            m_pLocalQuery->Close();
            m_pLocalQuery->SetSQL(m_sMdbSelectByPkSQL);
            m_pOraQuery->Open();
            bool bSame = false;
            int iCheckedCount = 0;
            while(m_pOraQuery->Next())
            {
                CHECK_RET(SetLocalKeyParam(m_pOraQuery, m_pLocalQuery), "SetLocalKeyParam() failed.");//���ñ���������SQL����������ֵ��
                m_pLocalQuery->Open();//������������������ȡһ����¼
                if (m_pLocalQuery->Next())
                {
                    CHECK_RET(CompareOneOraRcd(m_pOraQuery, m_pLocalQuery, bSame), "CompareOneRcd() failed.");//�Ƚ�������¼�Ƿ���ͬ
                    if (!bSame)
                    {
                        CHECK_RET(SaveRecord(m_pOraQuery, MDB_DIFF_DATA_NOT_SAME), "SaveRecord failed.");//����Զ˻�����
                    }
                    else
                    {
                        CHECK_RET(SaveRecord(m_pOraQuery, MDB_DIFF_DATA_SAME), "SaveRecord failed.");//����Զ˻�����
                    }
                }
                else
                {
                    CHECK_RET(SaveRecord(m_pOraQuery, MDB_DIFF_LOCAL_NOT_EXIST), "RecordDiffRcd() failed.");//��¼��ͬ����
                }
                iCheckedCount++;
                if (iCheckedCount%10000 == 0)
                {
                    TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %d ] records compared.", m_iThreadNO, m_pTable->sTableName, iCheckedCount);
                }
            }
            TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %d ] records compared.", m_iThreadNO, m_pTable->sTableName, iCheckedCount);
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            return iRet;
    }

    int TMdbCheckThread::CheckLocalData()
    {
        int iRet = 0;
        CHECK_OBJ(m_pPkFile);
        //TADD_NORMAL("Check local data ...");
        int iNotExist = 0;
        try
        {
            m_pLocalQuery->Close();
            m_pLocalQuery->SetSQL(m_sMdbSelectAllSQL);
            std::string strPK;
            int iNo = 0;
            TMDBDiffRcd tDiffRcd;
            m_pLocalQuery->Open();
            while(m_pLocalQuery->Next())
            {
                strPK.clear();
                for(int n=0; n<m_pTable->m_tPriKey.iColumnCounts; ++n)
                {
                    if (n!=0)
                    {
                        strPK.append(",");
                    }
                    iNo = m_pTable->m_tPriKey.iColumnNo[n];
                    strPK.append(m_pLocalQuery->Field(m_pTable->tColumn[iNo].sName).AsString());
                }
                if (!m_pPkFile->IsPkExist(strPK))
                {
                    CHECK_RET(SaveDiffRcd(strPK, MDB_DIFF_PEER_NOT_EXIST), "SaveDiffRcd failed");
                    iNotExist++;
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %d ] records not in peer.", m_iThreadNO, m_pTable->sTableName,  iNotExist);
            return iRet;
    }

    int TMdbCheckThread::ReCompare()
    {
        int iRet = 0;
        int iCmpTimes = 0;
        while(m_iDiffRcd > 0 && iCmpTimes < m_pCheckConfig->m_tCheckInfo.m_iCmpTimes)
        {
            if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_MDB)
            {
                CHECK_RET(ReCompareWithPeer(), "ReCompareWithPeer() failed.");
            }
            else
            {
                CHECK_RET(ReCompareWithOra(), "ReCompareWithOra() failed.");
            }

            iCmpTimes++;
            TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %d ] records different with peer after [%d] times compare", m_iThreadNO, m_pTable->sTableName,  m_iDiffRcd, iCmpTimes);

            if (m_iDiffRcd > 0 && iCmpTimes< m_pCheckConfig->m_tCheckInfo.m_iCmpTimes)
            {
                TADD_NORMAL( "THREAD [ %d ] TABLE [%s]:  Will compare again after %d s.", m_iThreadNO, m_pTable->sTableName,  m_pCheckConfig->m_tCheckInfo.m_iInterval);
                TMdbDateTime::Sleep(m_pCheckConfig->m_tCheckInfo.m_iInterval);
            }
        }
        return iRet;
    }

    int TMdbCheckThread::ReCompareWithPeer()
    {
        int iRet = 0;
        TADD_NORMAL("Compare again with Peer MDB ...");
        bool bSame = false;
        TMDBDiffRcd tDiffRcd;
        try
        {
            m_pPeerQuery->Close();
            m_pPeerQuery->SetSQL(m_sMdbSelectByPkSQL);
            m_pLocalQuery->Close();
            m_pLocalQuery->SetSQL(m_sMdbSelectByPkSQL);

            m_pDiffFile->SeekSetBegin();
            while(m_pDiffFile->GetNextDiffRcd(&tDiffRcd))
            {
                if (tDiffRcd.cRecheckSame == 'Y')//�ٴαȽ��Ѿ���ͬ
                {
                    continue;
                }

                CHECK_RET(SetPeesPKParam(tDiffRcd.strPk), "SetPeesPKParam failed.");
                CHECK_RET(SetLocalPKParam(tDiffRcd.strPk), "SetLocalPKParam failed");
                m_pPeerQuery->Open();
                m_pLocalQuery->Open();
                if (m_pPeerQuery->Next())//�Զ˴���
                {
                    if (m_pLocalQuery->Next())//���ش���
                    {
                        CHECK_RET(CompareOneMdbRcd(m_pPeerQuery, m_pLocalQuery, bSame), "CompareOneMdbRcd failed.");
                        if (bSame)//�ٴαȽ���ͬ
                        {
                            m_pDiffFile->UpdateDiffRcdSame();
                            m_iDiffRcd--;
                        }
                    }
                    else//���ز�����
                    {
                        if (tDiffRcd.cDiffType != MDB_DIFF_LOCAL_NOT_EXIST)
                        {
                            m_pDiffFile->UpdateDiffType(MDB_DIFF_LOCAL_NOT_EXIST);
                        }   
                    }                        
                }
                else //�Զ˲�����
                {
                    if (m_pLocalQuery->Next())//���ش���
                    {
                        if (tDiffRcd.cDiffType != MDB_DIFF_PEER_NOT_EXIST)
                        {
                            m_pDiffFile->UpdateDiffType(MDB_DIFF_PEER_NOT_EXIST);
                        }
                    }
                    else//���ز�����
                    {
                        if (tDiffRcd.cDiffType != MDB_DIFF_BOTH_NOT_EXIST)
                        {
                            m_pDiffFile->UpdateDiffType(MDB_DIFF_BOTH_NOT_EXIST);
                            m_pDiffFile->UpdateDiffRcdSame();
                            m_iDiffRcd--;
                        }
                    }    
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            return iRet;
    }

    int TMdbCheckThread::ReCompareWithOra()
    {
        int iRet = 0;

        bool bSame = false;
        TMDBDiffRcd tDiffRcd;
        try
        {
            m_pOraQuery->Close();
            m_pOraQuery->SetSQL(m_sOraSelectByPkSQL);
            SetCompexParam(m_pOraQuery);
            m_pLocalQuery->Close();
            m_pLocalQuery->SetSQL(m_sMdbSelectByPkSQL);

            m_pDiffFile->SeekSetBegin();
            while(m_pDiffFile->GetNextDiffRcd(&tDiffRcd))
            {
                if (tDiffRcd.cRecheckSame == 'Y')//�ٴαȽ��Ѿ���ͬ
                {
                    continue;
                }

                CHECK_RET(SetOraPKParam(tDiffRcd.strPk), "SetPeesPKParam failed.");
                CHECK_RET(SetLocalPKParam(tDiffRcd.strPk), "SetLocalPKParam failed");
                m_pOraQuery->Open();
                m_pLocalQuery->Open();
                if (m_pOraQuery->Next())//�Զ˴���
                {
                    if (m_pLocalQuery->Next())//���ش���
                    {
                        CHECK_RET(CompareOneOraRcd(m_pOraQuery, m_pLocalQuery, bSame), "CompareOneMdbRcd failed.");
                        if (bSame)//�ٴαȽ���ͬ
                        {
                            m_pDiffFile->UpdateDiffRcdSame();
                            m_iDiffRcd--;
                        }
                    }
                    else//���ز�����
                    {
                        if (tDiffRcd.cDiffType != MDB_DIFF_LOCAL_NOT_EXIST)
                        {
                            m_pDiffFile->UpdateDiffType(MDB_DIFF_LOCAL_NOT_EXIST);
                        }   
                    }                        
                }
                else //�Զ˲�����
                {
                    if (m_pLocalQuery->Next())//���ش���
                    {
                        if (tDiffRcd.cDiffType != MDB_DIFF_PEER_NOT_EXIST)
                        {
                            m_pDiffFile->UpdateDiffType(MDB_DIFF_PEER_NOT_EXIST);
                        }
                    }
                    else//���ز�����
                    {
                        if (tDiffRcd.cDiffType != MDB_DIFF_BOTH_NOT_EXIST)
                        {
                            m_pDiffFile->UpdateDiffType(MDB_DIFF_BOTH_NOT_EXIST);
                            m_pDiffFile->UpdateDiffRcdSame();
                            m_iDiffRcd--;
                        }
                    }    
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            return iRet;
    }

    int TMdbCheckThread::SetSelectAllSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_sMdbSelectAllSQL[0] = '\0';
     
        //ƴ�Ӵ�mdb�����������ݵ�SQL
        bool bFlag = false;
        snprintf(m_sMdbSelectAllSQL, sizeof(m_sMdbSelectAllSQL),"SELECT ");
        for(int j=0; j<m_pTable->iColumnCounts; ++j)
        {
            if(m_pTable->tColumn[j].iDataType == DT_Blob)
            {
                continue;
            }
            if(bFlag == false)
            {
                snprintf(m_sMdbSelectAllSQL + strlen(m_sMdbSelectAllSQL), sizeof(m_sMdbSelectAllSQL)- strlen(m_sMdbSelectAllSQL), " %s ", m_pTable->tColumn[j].sName);      
                bFlag = true;	             
            }
            else
            {
                snprintf(m_sMdbSelectAllSQL + strlen(m_sMdbSelectAllSQL), sizeof(m_sMdbSelectAllSQL) -strlen(m_sMdbSelectAllSQL), ", %s ", m_pTable->tColumn[j].sName);            
            }	
        }

        snprintf(m_sMdbSelectAllSQL + strlen(m_sMdbSelectAllSQL), sizeof(m_sMdbSelectAllSQL) -strlen(m_sMdbSelectAllSQL), " FROM %s ", m_pTable->sTableName);

        if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_ORACLE)//��oracle�Ƚϣ����ô�oracle���ص�sql
        {
            CHECK_RET(SetOraSelectAllSQL(), "SetOraSelectAllSQL failed.");
        }

        //�û�������filter sql
        if (!m_pCheckConfig->m_tDsn.m_strFilterSql.empty())    //����where����
        {
            snprintf(m_sMdbSelectAllSQL + strlen(m_sMdbSelectAllSQL), sizeof(m_sMdbSelectAllSQL) - strlen(m_sMdbSelectAllSQL), " WHERE %s", m_pCheckConfig->m_tDsn.m_strFilterSql.c_str());
        }

        TADD_FUNC("SelectMdbAllSQL = [%s]", m_sMdbSelectAllSQL);

        TADD_FUNC("Finish.");
        return iRet;

    }

    int TMdbCheckThread::SetOraSelectAllSQL()
    {
        int iRet = 0;
        m_sOraSelectAllSQL[0] = '\0';
        //ƴ�Ӵ�oracle�����������ݵ�sql
        if (m_pTableToCmp->m_bIsQdgTable)//qdg�ı�
        {
            snprintf(m_sOraSelectAllSQL, sizeof(m_sOraSelectAllSQL), "SELECT * FROM (%s) ", m_pTableToCmp->m_strLoadSql.c_str());
            //if (m_pCheckConfig->m_tDsn.m_strFilterSql.size()>0)//����filter sql
            //{
            //    snprintf(m_sOraSelectAllSQL + strlen(m_sOraSelectAllSQL), sizeof(m_sOraSelectAllSQL) - strlen(m_sOraSelectAllSQL), " WHERE %s", m_pCheckConfig->m_tDsn.m_strFilterSql.c_str());
            //}
        }
        else if (m_pTable->iRepAttr == REP_FROM_DB&&m_pTable->m_sLoadSQL[0] != '\0')//oracle2mdb�ı����loadsql
        {
        if (0 == m_pTable->iLoadType)//��AsStringģʽ����oracle�ı�
            {
            TADD_NORMAL("Table[%s]'s Load-type is [%d]. Must set load-sql in the configuration file of mdbDataCheck.", m_pTable->sTableName, m_pTable->iLoadType);
            return -1;
            }
            else
            {
                snprintf(m_sOraSelectAllSQL, sizeof(m_sOraSelectAllSQL), "SELECT * FROM (%s) ", m_pTable->m_sLoadSQL);
            }
        }
        else
        {
            SAFESTRCPY(m_sOraSelectAllSQL, sizeof(m_sOraSelectAllSQL), m_sMdbSelectAllSQL);
        }
        // ����Filter�������������м�¼
        if ((m_pTable->iRepAttr == REP_FROM_DB && m_pTable->m_sLoadSQL[0] == '\0' && m_pTable->m_sFilterSQL[0] != '\0'))//oracle2mdb�ı�������filtersql
        {
            snprintf(m_sOraSelectAllSQL + strlen(m_sOraSelectAllSQL), sizeof(m_sOraSelectAllSQL) - strlen(m_sOraSelectAllSQL), " WHERE %s ", m_pTable->m_sFilterSQL);
            if (!m_pCheckConfig->m_tDsn.m_strFilterSql.empty())    
            {
                snprintf(m_sOraSelectAllSQL + strlen(m_sOraSelectAllSQL), sizeof(m_sOraSelectAllSQL) - strlen(m_sOraSelectAllSQL), " AND %s", m_pCheckConfig->m_tDsn.m_strFilterSql.c_str());
            }
        }
        else
        {
            if (!m_pCheckConfig->m_tDsn.m_strFilterSql.empty())    //����where����
            {
                snprintf(m_sOraSelectAllSQL + strlen(m_sOraSelectAllSQL), sizeof(m_sOraSelectAllSQL) - strlen(m_sOraSelectAllSQL), " WHERE %s", m_pCheckConfig->m_tDsn.m_strFilterSql.c_str());
            }
        }

    TADD_FUNC("SelectOraAllSQL = [%s]", m_sOraSelectAllSQL);
        return iRet;
    }
    int TMdbCheckThread::SetSelectByPKSQL()
    {
        int iRet = 0;
        
        m_sMdbSelectByPkSQL[0] = '\0';
        bool bFlag = false;
        snprintf(m_sMdbSelectByPkSQL, MAX_SQL_LEN, "SELECT ");
        for(int j=0; j<m_pTable->iColumnCounts; ++j)
        {
            if (m_pTableToCmp!=NULL)//���бȽϲ��������Ƚ�blob����
            {
                if(m_pTable->tColumn[j].iDataType == DT_Blob)
                {
                    continue;
                }
            }
           
            if(bFlag == false)
            {
                snprintf(m_sMdbSelectByPkSQL + strlen(m_sMdbSelectByPkSQL), sizeof(m_sMdbSelectByPkSQL) - strlen(m_sMdbSelectByPkSQL), " %s ", m_pTable->tColumn[j].sName);
                bFlag = true;	              
            }
            else
            {
                snprintf(m_sMdbSelectByPkSQL + strlen(m_sMdbSelectByPkSQL), sizeof(m_sMdbSelectByPkSQL) - strlen(m_sMdbSelectByPkSQL), ", %s ", m_pTable->tColumn[j].sName);
            }	
        }

        snprintf(m_sMdbSelectByPkSQL + strlen(m_sMdbSelectByPkSQL), sizeof(m_sMdbSelectByPkSQL) -strlen(m_sMdbSelectByPkSQL), " FROM %s WHERE ", m_pTable->sTableName);

        int iNo = 0;
        for(int j=0; j<m_pTable->m_tPriKey.iColumnCounts; ++j)//��������ȡһ����¼
        {
            iNo = m_pTable->m_tPriKey.iColumnNo[j];
            if( j == 0)
            {
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sMdbSelectByPkSQL + strlen(m_sMdbSelectByPkSQL), sizeof(m_sMdbSelectByPkSQL) - strlen(m_sMdbSelectByPkSQL), "  %s = to_date(:%s, 'YYYYMMDDHH24MISS')  ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
                else
                {
                    snprintf(m_sMdbSelectByPkSQL + strlen(m_sMdbSelectByPkSQL), sizeof(m_sMdbSelectByPkSQL) - strlen(m_sMdbSelectByPkSQL), "  %s = :%s ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
            }
            else
            {	
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sMdbSelectByPkSQL + strlen(m_sMdbSelectByPkSQL), sizeof(m_sMdbSelectByPkSQL) - strlen(m_sMdbSelectByPkSQL), " AND %s = to_date(:%s, 'YYYYMMDDHH24MISS')  ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
                else
                {
                    snprintf(m_sMdbSelectByPkSQL + strlen(m_sMdbSelectByPkSQL), sizeof(m_sMdbSelectByPkSQL) - strlen(m_sMdbSelectByPkSQL), " AND %s = :%s ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
            }	
        }

        TADD_FUNC("MdbSelectByPkSQL = [%s]", m_sMdbSelectByPkSQL);

        if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_ORACLE) //��oracle���бȽϣ����ô�oracle��������ȡ���ݵ�sql
        {
            CHECK_RET(SetOraSelectByPKSQL(), "SetOraSelectByPKSQL() failed.");
        }    
       
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbCheckThread::SetOraSelectByPKSQL()
    {
        int iRet = 0;
        m_bOraSelectNoBlob = false;
        m_sOraSelectByPkSQL[0] = '\0';
        if (m_pTableToCmp!=NULL && m_pTableToCmp->m_bIsQdgTable)//qdg�����ݱȽ�ʱ��sql
        {
            snprintf(m_sOraSelectByPkSQL, sizeof(m_sOraSelectByPkSQL), "SELECT * FROM (%s) ", m_pTableToCmp->m_strLoadSql.c_str());
        }
        else if (m_pTableToRestore!=NULL && m_pTableToRestore->m_bIsQdgTable)//qdg�����ݻָ�ʱ��sql
        {
            snprintf(m_sOraSelectByPkSQL, sizeof(m_sOraSelectByPkSQL), "SELECT * FROM (%s) ", m_pTableToRestore->m_strLoadSql.c_str());
        }
    else if (m_pTable->iRepAttr == REP_FROM_DB  && m_pTable->m_sLoadSQL[0] != '\0')
    {
        if (0==m_pTable->iLoadType)
        {
            TADD_NORMAL("Table[%s]'s Load-type is [%d]. Must set load-sql in the configuration file of mdbDataCheck.", m_pTable->sTableName, m_pTable->iLoadType);
            return -1;
        }
        else
        {
            snprintf(m_sOraSelectByPkSQL, sizeof(m_sOraSelectByPkSQL), "SELECT * FROM (%s) ", m_pTable->m_sLoadSQL);
        }
    }


        if (m_sOraSelectByPkSQL[0] != '\0')
        {
            try
            {
                //����������Ϣ
                m_pOraQuery->Close();
                m_pOraQuery->SetSQL(m_sOraSelectByPkSQL);
                SetCompexParam(m_pOraQuery);
                m_pOraQuery->Open();
                m_pOraQuery->Next();
                for (int i = 0; i<m_pTable->m_tPriKey.iColumnCounts; i++)
                {
                    if (0 == i)
                    {
                        if (m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].iDataType == DT_DateStamp)
                        {
                            snprintf(m_sOraSelectByPkSQL + strlen(m_sOraSelectByPkSQL), sizeof(m_sOraSelectByPkSQL) - strlen(m_sOraSelectByPkSQL), " WHERE %s = to_date(:%s, 'YYYYMMDDHH24MISS')", 
                                m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName(), m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName());
                        }
                        else
                        {
                            snprintf(m_sOraSelectByPkSQL + strlen(m_sOraSelectByPkSQL), sizeof(m_sOraSelectByPkSQL) - strlen(m_sOraSelectByPkSQL), " WHERE %s = :%s", 
                                m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName(), m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName());
                        }
                    }
                    else
                    {
                        if (m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].iDataType == DT_DateStamp)
                        {
                            snprintf(m_sOraSelectByPkSQL + strlen(m_sOraSelectByPkSQL), sizeof(m_sOraSelectByPkSQL) - strlen(m_sOraSelectByPkSQL), " AND %s = to_date(:%s, 'YYYYMMDDHH24MISS')", 
                                m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName(), m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName());
                        }
                        else
                        {
                            snprintf(m_sOraSelectByPkSQL + strlen(m_sOraSelectByPkSQL), sizeof(m_sOraSelectByPkSQL) - strlen(m_sOraSelectByPkSQL), " AND %s = :%s", 
                                m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName(), m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).GetFieldName());
                        }
                    }
                }
            }
            CATCH_MDB_DATA_CHECK_EXEPTION
            
        }
        else//���mdb�а�ס����ȡ����sqlһ��
        {
            m_bOraSelectNoBlob = true;//sql��һ��������blob�ֶ�
            SAFESTRCPY(m_sOraSelectByPkSQL, sizeof(m_sOraSelectByPkSQL), m_sMdbSelectByPkSQL);
        }

    TADD_FUNC("OraSelectByPkSQL = [%s]", m_sOraSelectByPkSQL);
        return iRet;
    }
    /******************************************************************************
    * ��������	:  SetRestoreInsertSQL()
    * ��������	:  ���ûָ�����ʱ��Insert SQL
    * ����		:  pTable ��ָ��
    * ���		:  sInsertSQL ����SQL
    * ����ֵ	:  �ɹ����أ����򷵻ط�
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::SetInsertSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        snprintf(m_sInsertSQL, MAX_SQL_LEN, "INSERT INTO %s(", m_pTable->sTableName);

        bool bIFlag = false;
        bool bToOracle= false;
        if (m_pCheckConfig->m_tDsn.m_eType == E_DSN_ORACLE && m_pTable->iRepAttr == REP_TO_DB)
        {
            bToOracle = true;
        }

        for(int j=0; j<m_pTable->iColumnCounts; ++j)
        {
            //if(m_pTable->tColumn[j].iDataType == DT_Blob)
            //{
            //    continue;
            //}
            if(bIFlag == false)
            {
                snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), " %s", m_pTable->tColumn[j].sName);
                bIFlag = true;	
            }
            else
            {
                snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), ", %s ", m_pTable->tColumn[j].sName);
            }	

        }

        snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), ") VALUES(");

        bIFlag = false;
        for(int j=0; j<m_pTable->iColumnCounts; ++j)
        {
            //if(m_pTable->tColumn[j].iDataType == DT_Blob)
            //{
            //    continue;
            //}

            if(bIFlag == false)
            {
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), " to_date(:%s, 'YYYYMMDDHH24MISS')", m_pTable->tColumn[j].sName);
                }
                else if (bToOracle && m_pTable->tColumn[j].iDataType == DT_Blob)//��oracle����blob�ֶ�
                {
                    snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), " rawtohex(:%s)", m_pTable->tColumn[j].sName);
                }
                else
                {
                    snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), " :%s", m_pTable->tColumn[j].sName);
                }

                bIFlag = true;	
            }
            else
            {
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), ", to_date(:%s, 'YYYYMMDDHH24MISS')", m_pTable->tColumn[j].sName);
                }
                else if (bToOracle && m_pTable->tColumn[j].iDataType == DT_Blob)
                {
                    snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), " , rawtohex(:%s)", m_pTable->tColumn[j].sName);
                }
                else
                {
                    snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), ", :%s ", m_pTable->tColumn[j].sName);
                }
            }	
        }
        snprintf(m_sInsertSQL + strlen(m_sInsertSQL), MAX_SQL_LEN - strlen(m_sInsertSQL), ")");

        TADD_FUNC("INSERT SQL = [%s]", m_sInsertSQL);

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetRestoreUpdateSQL()
    * ��������	:  ���ûָ�����ʱ��Update SQL
    * ����		:  pTable ��ָ��
    * ���		:  sUpateSQL ����SQL
    * ����ֵ	:  �ɹ����أ����򷵻ط�
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::SetUpdateSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        snprintf(m_sUpdateSQL, MAX_SQL_LEN, "UPDATE %s SET", m_pTable->sTableName);
        bool bUFlag = false;
        for(int j=0; j<m_pTable->iColumnCounts; ++j)
        {
            if(m_pTable->tColumn[j].iDataType == DT_Blob)
            {
                continue;
            }
            if (!CheckColumIsPK(j))
            {
                if (bUFlag == false)
                {
                    if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                    {
                        snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL), MAX_SQL_LEN -strlen(m_sUpdateSQL), " %s=to_date(:%s, 'YYYYMMDDHH24MISS')", m_pTable->tColumn[j].sName, m_pTable->tColumn[j].sName);
                    }
                    else
                    {
                        snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL), MAX_SQL_LEN -strlen(m_sUpdateSQL), " %s=:%s", m_pTable->tColumn[j].sName, m_pTable->tColumn[j].sName);
                    }

                    bUFlag = true;
                }
                else
                {
                    if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                    {
                        snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL), MAX_SQL_LEN -strlen(m_sUpdateSQL), " , %s=to_date(:%s, 'YYYYMMDDHH24MISS')", m_pTable->tColumn[j].sName, m_pTable->tColumn[j].sName);
                    }
                    else
                    {
                        snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL), MAX_SQL_LEN -strlen(m_sUpdateSQL), " , %s=:%s", m_pTable->tColumn[j].sName, m_pTable->tColumn[j].sName);
                    } 
                }
            }
        }

        int iNo = 0;
        for(int j=0; j<m_pTable->m_tPriKey.iColumnCounts; ++j)//��������ȡһ����¼
        {
            iNo = m_pTable->m_tPriKey.iColumnNo[j];
            if( j == 0)
            {
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sUpdateSQL + strlen(m_sUpdateSQL), MAX_SQL_LEN - strlen(m_sUpdateSQL), " WHERE %s =  to_date(:%s, 'YYYYMMDDHH24MISS') ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
                else
                {
                    snprintf(m_sUpdateSQL + strlen(m_sUpdateSQL), MAX_SQL_LEN - strlen(m_sUpdateSQL), " WHERE %s = :%s ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }  
            }
            else
            {	
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sUpdateSQL + strlen(m_sUpdateSQL), MAX_SQL_LEN - strlen(m_sUpdateSQL), " AND %s = to_date(:%s, 'YYYYMMDDHH24MISS') ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
                else
                {
                    snprintf(m_sUpdateSQL + strlen(m_sUpdateSQL), MAX_SQL_LEN - strlen(m_sUpdateSQL), " AND %s = :%s ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }

            }	
        }

        TADD_FUNC("UPDATE SQL = [%s]", m_sUpdateSQL);

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetRestoreUpdateSQL()
    * ��������	:  ���ûָ�����ʱ��Update SQL
    * ����		:  pTable ��ָ��
    * ���		:  sUpateSQL ����SQL
    * ����ֵ	:  �ɹ����أ����򷵻ط�
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::SetDeleteSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        snprintf(m_sDeleteSQL, MAX_SQL_LEN, "DELETE FROM %s ", m_pTable->sTableName);
        int iNo = 0;
        for(int j=0; j<m_pTable->m_tPriKey.iColumnCounts; ++j)//��������ȡһ����¼
        {
            iNo = m_pTable->m_tPriKey.iColumnNo[j];
            if( j == 0)
            {
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sDeleteSQL + strlen(m_sDeleteSQL), MAX_SQL_LEN - strlen(m_sDeleteSQL), " WHERE %s = to_date(:%s, 'YYYYMMDDHH24MISS') ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
                else
                {
                    snprintf(m_sDeleteSQL + strlen(m_sDeleteSQL), MAX_SQL_LEN - strlen(m_sDeleteSQL), " WHERE %s = :%s ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
            }
            else
            {	
                if (m_pTable->tColumn[j].iDataType == DT_DateStamp)
                {
                    snprintf(m_sDeleteSQL + strlen(m_sDeleteSQL), MAX_SQL_LEN - strlen(m_sDeleteSQL), " AND %s = to_date(:%s, 'YYYYMMDDHH24MISS') ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
                else
                {
                    snprintf(m_sDeleteSQL + strlen(m_sDeleteSQL), MAX_SQL_LEN - strlen(m_sDeleteSQL), " AND %s = :%s ", m_pTable->tColumn[iNo].sName, m_pTable->tColumn[iNo].sName);
                }
            }	
        }

        TADD_FUNC("DELETE SQL = [%s]", m_sDeleteSQL);

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetLocalKeyParam()
    * ��������	:  ���ݶԶ˲�ѯ����һ����¼�����ñ��ز�ѯָ���SQL����������ȡֵ��
    * ����		:  pTable ��ָ��
    * ����		:  pPeerQuery ��ѯָ�룬ָ��Զ˲�ѯ����һ����¼
    * ����		:  pLocalQuery ���ز�ѯָ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻ط�0
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::SetLocalKeyParam(TMdbClientQuery *pPeerQuery, TMdbQuery *pLocalQuery)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iNo = 0;
        try
        {
            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                iNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(m_pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, pPeerQuery->Field(m_pTable->tColumn[iNo].sName).AsInteger());
                }
                else if(m_pTable->tColumn[iNo].iDataType == DT_DateStamp)
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, pPeerQuery->Field(m_pTable->tColumn[iNo].sName).AsDateTimeString());
                }
                else
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, pPeerQuery->Field(m_pTable->tColumn[iNo].sName).AsString());
                }
            }//end for(int j=0; j<(pTable->m_tPriKey).iColumnCounts; ++j)
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetLocalKeyParam()
    * ��������	:  ����Oracle��ѯ����һ����¼�����ñ��ز�ѯָ���SQL����������ȡֵ��
    * ����		:  pTable ��ָ��
    * ����		:  pPeerQuery ��ѯָ�룬ָ��oracle��ѯ����һ����¼
    * ����		:  pLocalQuery ���ز�ѯָ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻ط�0
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::SetLocalKeyParam(TMDBDBQueryInterface *pPeerQuery, TMdbQuery *pLocalQuery)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iNo = 0;
        try
        {
            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                iNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(m_pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, pPeerQuery->Field(m_pTable->tColumn[iNo].sName).AsInteger());
                }
                else if(m_pTable->tColumn[iNo].iDataType == DT_DateStamp)
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, pPeerQuery->Field(m_pTable->tColumn[iNo].sName).AsDateTimeString());
                }
                else
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, pPeerQuery->Field(m_pTable->tColumn[iNo].sName).AsString());
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  CompareOneRcd()
    * ��������	:  �Ƚ�������ѯָ��ָ��ļ�¼�Ƿ���ͬ
    * ����		:  pClientQuery ��ѯָ��
    * ����		:  pLocalQuery ���ز�ѯָ��
    * ���		:  bSame �Ƿ���ͬ
    * ����ֵ	:  �ɹ�����0�����򷵻ط�0
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::CompareOneMdbRcd(TMdbClientQuery *pPeerQuery, TMdbQuery *pLocalQuery, bool &bSame)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        bSame = true;
        try
        {
            for(int iPos=0; iPos<pPeerQuery->FieldCount(); ++iPos)//�����Ƚ�������
            {
                if(pPeerQuery->Field(iPos).isNULL())
                {
                    if (!pLocalQuery->Field(iPos).isNULL())
                    {
                    if (TMdbCheckDataMgr::m_bDetail)
                    {
                        TADD_NORMAL("[%s]: %s [NULL] : [%s]", m_pTable->sTableName, pLocalQuery->Field(iPos).GetName(), pLocalQuery->Field(iPos).AsString());
                    }
                        bSame = false;
                        break;
                    }
                }
                else
                {
                    if(strcmp(pPeerQuery->Field(iPos).AsString(), pLocalQuery->Field(iPos).AsString()) != 0)
                    {
                    if (TMdbCheckDataMgr::m_bDetail)
                    {
                        TADD_NORMAL("[%s]: %s [%s] : [%s]", m_pTable->sTableName, pLocalQuery->Field(iPos).GetName(), pPeerQuery->Field(iPos).AsString(), pLocalQuery->Field(iPos).AsString());
                    }
                        bSame = false;
                        break;	
                    }
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  CompareOneRcd()
    * ��������	:  �Ƚ�������ѯָ��ָ��ļ�¼�Ƿ���ͬ
    * ����		:  pPeerQuery Oracle��ѯָ��
    * ����		:  pLocalQuery ���ز�ѯָ��
    * ���		:  bSame �Ƿ���ͬ
    * ����ֵ	:  �ɹ�����0�����򷵻ط�0
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::CompareOneOraRcd(TMDBDBQueryInterface *pOraQuery, TMdbQuery *pLocalQuery, bool &bSame)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        bSame = true;
        int iBlobNum = 0;//���Ǵ�oracle��loadsql�У�������blob�ֶ�

        try
        {
            int iPos = -1;
            for(int j=0; j<m_pTable->iColumnCounts; ++j)//�����Ƚ�������
            {
                if(m_pTable->tColumn[j].iDataType == DT_Blob)
                {
                    if (!m_bOraSelectNoBlob)
                    {
                        iBlobNum++;
                    }
                    continue;
                }
                iPos++;
                if(pOraQuery->Field(iPos).isNULL())
                {
                    if (!(pLocalQuery->Field(iPos-iBlobNum).isNULL()|| strlen(pLocalQuery->Field(iPos-iBlobNum).AsString()) == 0))
                    {
                    if (TMdbCheckDataMgr::m_bDetail)
                    {
                        TADD_NORMAL("[%s]: %s [NULL] : [%s]", m_pTable->sTableName, pLocalQuery->Field(iPos-iBlobNum).GetName(), pLocalQuery->Field(iPos-iBlobNum).AsString());
                    }
                        bSame = false;
                        break;
                    }
                }
                else
                {
                    if(m_pTable->tColumn[iPos].iDataType == DT_Int)
                    {
                        if(pOraQuery->Field(iPos).AsInteger() != pLocalQuery->Field(iPos-iBlobNum).AsInteger())
                        {
                        if (TMdbCheckDataMgr::m_bDetail)
                        {
                            TADD_NORMAL("[%s]: %s [%d] : [%d]", m_pTable->sTableName, pLocalQuery->Field(iPos-iBlobNum).GetName(), pOraQuery->Field(iPos).AsInteger(), pLocalQuery->Field(iPos-iBlobNum).AsInteger());
                        }
                            bSame = false;
                            break;	
                        }
                    }
                    else if (m_pTable->tColumn[iPos].iDataType == DT_DateStamp)
                    {
                        if(strcmp(pOraQuery->Field(iPos).AsDateTimeString(), pLocalQuery->Field(iPos-iBlobNum).AsDateTimeString()) != 0)
                        {
                        if (TMdbCheckDataMgr::m_bDetail)
                        {
                            TADD_NORMAL("[%s]: %s [%s] : [%s]", m_pTable->sTableName, pLocalQuery->Field(iPos-iBlobNum).GetName(), pOraQuery->Field(iPos).AsDateTimeString(), pLocalQuery->Field(iPos-iBlobNum).AsDateTimeString());
                        }
                            bSame = false;
                            break;	
                        }
                    }
                    else 
                    {
                        /*TMdbStrFunc::RightTrim(pOraQuery->Field(iPos).AsString());
                        TMdbStrFunc::RightTrim(pLocalQuery->Field(iPos-iBlobNum).AsString());*/

                        if(strcmp(pOraQuery->Field(iPos).AsString(),pLocalQuery->Field(iPos-iBlobNum).AsString()) != 0)//������oracle���ֶ�Ϊchar�����
                        {
                        if (TMdbCheckDataMgr::m_bDetail)
                        {
                            TADD_NORMAL("[%s]: %s [%s] : [%s]", m_pTable->sTableName, pLocalQuery->Field(iPos-iBlobNum).GetName(), pOraQuery->Field(iPos).AsString(), pLocalQuery->Field(iPos-iBlobNum).AsString());
                        }
                            bSame = false;
                            break;
                        } 
                    }
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbCheckThread::SaveRecord(TMdbClientQuery *pPeerQuery, int iDiffType)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        m_strPK.clear();
        int iNo = 0;
        try
        {
            for(int n=0; n<m_pTable->m_tPriKey.iColumnCounts; ++n)
            {
                if (n != 0)
                {
                    m_strPK.append(",");//����֮���Զ��ŷָ�
                }
                iNo = m_pTable->m_tPriKey.iColumnNo[n];
                m_strPK.append(pPeerQuery->Field(m_pTable->tColumn[iNo].sName).AsString());
            }
            m_pPkFile->WritePk(m_strPK);//д�������ļ�

            if (MDB_DIFF_DATA_SAME != iDiffType)//���ݲ�ͬ��д�벻ͬ�ļ�
            {
                CHECK_RET(SaveDiffRcd(m_strPK, iDiffType), "SaveDiffRcd failed.");
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish");
        return iRet;
    }


    int TMdbCheckThread::SaveRecord(TMDBDBQueryInterface *pOraQuery, int iDiffType)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        m_strPK.clear();
        int iNo = 0;
        static int iCount = 0;
        try
        {
            for(int n=0; n<m_pTable->m_tPriKey.iColumnCounts; ++n)
            {
                if (n != 0)
                {
                    m_strPK.append(",");//����֮���Զ��ŷָ�
                }
                iNo = m_pTable->m_tPriKey.iColumnNo[n];
                m_strPK.append(pOraQuery->Field(m_pTable->tColumn[iNo].sName).AsString());
            }
            m_pPkFile->WritePk(m_strPK);//д�������ļ�

            if (MDB_DIFF_DATA_SAME != iDiffType)//���ݲ�ͬ��д�벻ͬ�ļ�
            {
                CHECK_RET(SaveDiffRcd(m_strPK, iDiffType), "SaveDiffRcd failed.");
            }
            iCount++;
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish");
        return iRet;
    }

    int TMdbCheckThread::SaveDiffRcd(std::string strPk, int iDiffType)
    {
        int iRet = 0;
        if (!m_bDiffExist)//��һ�α���ʱ�������ļ�ָ��
        {
            m_bDiffExist = true;
            m_pDiffFile = new(std::nothrow) TMdbDiffFile();
            CHECK_OBJ(m_pDiffFile);
            CHECK_RET(m_pDiffFile->Init(m_strPath, m_pTableToCmp->m_strTableName, m_pTableToCmp->m_iPkLen+MDB_DATA_CHECK_INT_BUF), "TMdbDiffFile init failed.");
        }

        TMDBDiffRcd tDiffRcd;
        tDiffRcd.cDiffType = (char)iDiffType;
        tDiffRcd.cRecheckSame = 'N';
        tDiffRcd.strPk = strPk;
        CHECK_RET(m_pDiffFile->WriteDiffRcd(&tDiffRcd), "WriteDiffRcd to file failed.");
        m_iDiffRcd++;
        return iRet;
    }

    /******************************************************************************
    * ��������	:  CheckColumIsPK()
    * ��������	:  ������Ƿ�Ϊ������
    * ����		:  pTable ����Ϣָ��
    * ����		:  iColumnIdx �����
    * ���		:  ��
    * ����ֵ	:  ��-true �� ��- false
    * ����		:  jiang.lili
    *******************************************************************************/
    bool TMdbCheckThread::CheckColumIsPK(const int iColumnIdx)
    {
        TADD_FUNC("Start.");
        CHECK_OBJ(m_pTable);
        bool bIsPK = false;

        for(int n=0; n<m_pTable->m_tPriKey.iColumnCounts; ++n)
        {
            int iNo = m_pTable->m_tPriKey.iColumnNo[n];
            if(iNo == iColumnIdx)
            {
                bIsPK = true;
                break;  
            }
        }

        TADD_FUNC("Finish.");

        return bIsPK;
    }

    void TMdbCheckThread::ClearCmpResult()
    {
        m_pTableToCmp = NULL;
        SAFE_DELETE(m_pPkFile);
        SAFE_DELETE(m_pDiffFile);
        m_eState = E_CHECK_THREAD_FREE;
        m_bDiffExist = false;
        m_iDiffRcd = 0;
        m_pTable = NULL;
        m_pTableToRestore = NULL;
    }

    int TMdbCheckThread::RestoreDataFromPeer()
    {
        int iRet = 0;
        TMdbQuery* pUpdateQuery = m_pLocalMdb->CreateDBQuery();
        TMdbQuery* pInsertQuery = m_pLocalMdb->CreateDBQuery();
        TMdbQuery* pDeleteQuery = m_pLocalMdb->CreateDBQuery();

        TMDBDiffRcd tDiffRcd;
        long long iCount = 0;
        try
        {
            m_pPeerQuery->Close();
            m_pPeerQuery->SetSQL(m_sMdbSelectByPkSQL);
            pUpdateQuery->SetSQL(m_sUpdateSQL, QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
            pInsertQuery->SetSQL(m_sInsertSQL, QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
            pDeleteQuery->SetSQL(m_sDeleteSQL, QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);

            while(m_pDiffFile->GetNextDiffRcd(&tDiffRcd))
            {
                if (tDiffRcd.cRecheckSame == 'Y')//�ٴαȽ��Ѿ���ͬ
                {
                    continue;
                }
                try
                {
                    switch(tDiffRcd.cDiffType)
                    {
                    case MDB_DIFF_DATA_NOT_SAME://���ݲ�һ��
                        {
                            CHECK_RET(SetPeesPKParam(tDiffRcd.strPk), "SetPeesPKParam failed.");
                            m_pPeerQuery->Open();
                            if (m_pPeerQuery->Next())
                            {
                                CHECK_RET(SetRestoreLocalParam(m_pPeerQuery, pUpdateQuery, E_UPDATE), "SetRestoreLocalParam update failed.");
                                pUpdateQuery->Execute();
                                pUpdateQuery->Commit();  
                                iCount++;
                            }
                            break;
                        }
                    case MDB_DIFF_LOCAL_NOT_EXIST://����������
                        {
                            CHECK_RET(SetPeesPKParam(tDiffRcd.strPk), "SetPeesPKParam failed.");
                            m_pPeerQuery->Open();
                            if (m_pPeerQuery->Next())
                            {
                                CHECK_RET(SetRestoreLocalParam(m_pPeerQuery, pInsertQuery, E_INSERT), "SetRestoreLocalParam update failed.");
                                pInsertQuery->Execute();
                                pInsertQuery->Commit();  
                                iCount++;
                            }
                            break;
                        }
                    case MDB_DIFF_PEER_NOT_EXIST://������������
                        {
                            /*
                            CHECK_RET(SetDeleteLocalParam(pDeleteQuery, tDiffRcd.strPk), "SetDeleteLocalParam failed.");
                            pDeleteQuery->Execute();
                            pDeleteQuery->Commit();
                            iCount++;
                            */
                            break;
                        }
                    default:
                        {
                            CHECK_RET(-1, "Unknown different type.");
                        }
                    }
                }
                catch(TMdbException &e)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"MdbException. \nERROR_MSG=%s\n ERROR_SQL=%s",e.GetErrMsg(), e.GetErrSql());
                }
                catch(TMDBDBExcpInterface &e)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"OracleException. \nERROR_CODE=%d\nERROR_MSG=%s\n ERROR_SQL=%s",e.GetErrCode(), e.GetErrMsg(), e.GetErrSql());
                }
                catch(...)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Unknown exception.");
                }

                if (iCount%10000 == 0)
                {
                    TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %lld ] records excuted.", m_iThreadNO, m_pTableToRestore->m_strTableName.c_str(), iCount);
                }
            }
            SAFE_DELETE(pDeleteQuery);
            SAFE_DELETE(pUpdateQuery);
            SAFE_DELETE(pInsertQuery);
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

        TADD_NORMAL_TO_CLI(FMT_CLI_OK, "THREAD [ %d ]: Restore [ %lld ] records in Table[%s]", m_iThreadNO, iCount, m_pTableToRestore->m_strTableName.c_str());
        return iRet;
    }

    int TMdbCheckThread::RestoreDataToOra()
    {
        int iRet = 0;
        TMDBDBQueryInterface* pUpdateQuery = m_pOraDB->CreateDBQuery();
        TMDBDBQueryInterface* pInsertQuery = m_pOraDB->CreateDBQuery();
        TMDBDBQueryInterface* pDeleteQuery = m_pOraDB->CreateDBQuery();

        TMDBDiffRcd tDiffRcd;
        long long iCount = 0;
        try
        {
            m_pLocalQuery->Close();
            m_pLocalQuery->SetSQL(m_sMdbSelectByPkSQL);
            pUpdateQuery->SetSQL(m_sUpdateSQL);
            pInsertQuery->SetSQL(m_sInsertSQL);
            pDeleteQuery->SetSQL(m_sDeleteSQL);
            while(m_pDiffFile->GetNextDiffRcd(&tDiffRcd))
            {
                if (tDiffRcd.cRecheckSame == 'Y')//�ٴαȽ��Ѿ���ͬ
                {
                    continue;
                }
                try
                {
                    switch(tDiffRcd.cDiffType)
                    {
                    case MDB_DIFF_DATA_NOT_SAME://���ݲ�һ��
                        {
                            CHECK_RET(SetLocalPKParam(tDiffRcd.strPk), "SetOraPKParam failed.");
                            m_pLocalQuery->Open();
                            if (m_pLocalQuery->Next())
                            {
                                CHECK_RET(SetRestoreOraParam(m_pLocalQuery, pUpdateQuery, E_UPDATE), "SetRestoreLocalParam update failed.");
                                pUpdateQuery->Execute();
                                pUpdateQuery->Commit();  
                                iCount++;
                            }
                            break;
                        }
                    case MDB_DIFF_LOCAL_NOT_EXIST://����������,oracle�����������ݣ���Ҫɾ��
                        {
                            /*
                            CHECK_RET(SetDeleteOraParam(pDeleteQuery, tDiffRcd.strPk), "SetDeleteLocalParam failed.");
                            pDeleteQuery->Execute();
                            pDeleteQuery->Commit();
                            iCount++;
                            */
                            break;
                            
                        }
                    case MDB_DIFF_PEER_NOT_EXIST://oracle�����ݲ����ڣ���Ҫ����
                        {
                            CHECK_RET(SetLocalPKParam(tDiffRcd.strPk), "SetOraPKParam failed.");
                            m_pLocalQuery->Open();
                            if (m_pLocalQuery->Next())
                            {
                                CHECK_RET(SetRestoreOraParam(m_pLocalQuery, pInsertQuery, E_INSERT), "SetRestoreOraParam update failed.");
                                pInsertQuery->Execute();
                                pInsertQuery->Commit();  
                                iCount++;
                            }
                            break;
                        }
                    default:
                        {
                            //CHECK_RET(-1, "Unknown different type.");
                            TADD_ERROR(ERROR_UNKNOWN,"Unknown different type");
                            continue;
                        }
                    }
                }
                catch(TMdbException &e)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"MdbException. \nERROR_MSG=%s\n ERROR_SQL=%s",e.GetErrMsg(), e.GetErrSql());
                }
                catch(TMDBDBExcpInterface &e)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"OracleException. \nERROR_CODE=%d\nERROR_MSG=%s\n ERROR_SQL=%s",e.GetErrCode(), e.GetErrMsg(), e.GetErrSql());
                }
                catch(...)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Unknown exception.");
                }

                if (iCount%10000 == 0)
                {
                    TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %lld ] records excuted.", m_iThreadNO, m_pTableToRestore->m_strTableName.c_str(), iCount);
                }
            }
            SAFE_DELETE(pDeleteQuery);
            SAFE_DELETE(pUpdateQuery);
            SAFE_DELETE(pInsertQuery);
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_NORMAL_TO_CLI(FMT_CLI_OK, "THREAD [ %d ]: Restore [ %lld ] records in Table[%s]", m_iThreadNO, iCount, m_pTableToRestore->m_strTableName.c_str());
            return iRet;
    }

    int TMdbCheckThread::RestoreDataFromOra()
    {
        int iRet = 0;
        TMdbQuery* pUpdateQuery = m_pLocalMdb->CreateDBQuery();
        TMdbQuery* pInsertQuery = m_pLocalMdb->CreateDBQuery();
        TMdbQuery* pDeleteQuery = m_pLocalMdb->CreateDBQuery();

        TMDBDiffRcd tDiffRcd;
        long long iCount = 0;
        try
        { 
            m_pOraQuery->Close();
            m_pOraQuery->SetSQL(m_sOraSelectByPkSQL);
            SetCompexParam(m_pOraQuery);
            //pUpdateQuery->SetSQL(m_sUpdateSQL);
           // pInsertQuery->SetSQL(m_sInsertSQL);
            //pDeleteQuery->SetSQL(m_sDeleteSQL);
            
            pUpdateQuery->SetSQL(m_sUpdateSQL, QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
            pInsertQuery->SetSQL(m_sInsertSQL, QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
            pDeleteQuery->SetSQL(m_sDeleteSQL, QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);

            while(m_pDiffFile->GetNextDiffRcd(&tDiffRcd))
            {
                if (tDiffRcd.cRecheckSame == 'Y')//�ٴαȽ��Ѿ���ͬ
                {
                    continue;
                }
                try
                {
                    switch(tDiffRcd.cDiffType)
                    {
                    case MDB_DIFF_DATA_NOT_SAME://���ݲ�һ��
                        {
                            CHECK_RET(SetOraPKParam(tDiffRcd.strPk), "SetOraPKParam failed.");
                            m_pOraQuery->Open();
                            if (m_pOraQuery->Next())
                            {
                                CHECK_RET(SetRestoreLocalParam(m_pOraQuery, pUpdateQuery, E_UPDATE), "SetRestoreLocalParam update failed.");
                                pUpdateQuery->Execute();
                                pUpdateQuery->Commit();  
                                iCount++;
                            }
                            break;
                        }
                    case MDB_DIFF_LOCAL_NOT_EXIST://����������
                        {
                            CHECK_RET(SetOraPKParam(tDiffRcd.strPk), "SetOraPKParam failed.");
                            m_pOraQuery->Open();
                            if (m_pOraQuery->Next())
                            {
                                CHECK_RET(SetRestoreLocalParam(m_pOraQuery, pInsertQuery, E_INSERT), "SetRestoreLocalParam update failed.");
                                pInsertQuery->Execute();
                                pInsertQuery->Commit();  
                                iCount++;
                            }
                            break;
                        }
                    case MDB_DIFF_PEER_NOT_EXIST://������������
                        {
                            /*    
                            CHECK_RET(SetDeleteLocalParam(pDeleteQuery, tDiffRcd.strPk), "SetDeleteLocalParam failed.");
                            pDeleteQuery->Execute();
                            pDeleteQuery->Commit();
                            iCount++;
                            */
                            break;
                        }
                    default:
                        {
                            CHECK_RET(-1, "Unknown different type.");
                        }
                    }
                }
                catch(TMdbException &e)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"MdbException. \nERROR_MSG=%s\n ERROR_SQL=%s",e.GetErrMsg(), e.GetErrSql());
                }
                catch(TMDBDBExcpInterface &e)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"OracleException. \nERROR_CODE=%d\nERROR_MSG=%s\n ERROR_SQL=%s",e.GetErrCode(), e.GetErrMsg(), e.GetErrSql());
                }
                catch(...)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Unknown exception.");
                }

                if (iCount%10000 == 0)
                {
                    TADD_NORMAL("THREAD [ %d ] TABLE [%s]: [ %lld ] records excuted.", m_iThreadNO, m_pTableToRestore->m_strTableName.c_str(), iCount);
                }
            }
            SAFE_DELETE(pDeleteQuery);
            SAFE_DELETE(pUpdateQuery);
            SAFE_DELETE(pInsertQuery);
        }
        CATCH_MDB_DATA_CHECK_EXEPTION
        TADD_NORMAL_TO_CLI(FMT_CLI_OK, "THREAD [ %d ]: Restore [ %lld ] records in Table[%s]", m_iThreadNO, iCount, m_pTableToRestore->m_strTableName.c_str());
            return iRet;
    }

    int TMdbCheckThread::SetPeesPKParam(std::string strPks)
    {
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(strPks.c_str(), ",");
        if ((int)(tSplit.GetFieldCount()) != m_pTable->m_tPriKey.iColumnCounts)
        {
            CHECK_RET(-1, "PK number in the file is wrong.");
        }
        int iNo = 0;
        try
        {
            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                iNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(m_pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    m_pPeerQuery->SetParameter(m_pTable->tColumn[iNo].sName, atoi(tSplit[(MDB_UINT32)i]));
                }
                else
                {
                    m_pPeerQuery->SetParameter(m_pTable->tColumn[iNo].sName, tSplit[(MDB_UINT32)i]);
                }
            }//end for(int j=0; j<(pTable->m_tPriKey).iColumnCounts; ++j)
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            return iRet;
    }

    int TMdbCheckThread::SetDeleteLocalParam(TMdbQuery* pLocalQuery, std::string strPks)
    {
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(strPks.c_str(), ",");
        if ((int)(tSplit.GetFieldCount()) != m_pTable->m_tPriKey.iColumnCounts)
        {
            CHECK_RET(-1, "PK number in the file is wrong.");
        }
        int iNo = 0;
        try
        {
            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                iNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(m_pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, atoi(tSplit[(MDB_UINT32)i]));
                }
                else
                {
                    pLocalQuery->SetParameter(m_pTable->tColumn[iNo].sName, tSplit[(MDB_UINT32)i]);
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            return iRet;
    }

    int TMdbCheckThread::SetOraPKParam(std::string strPks)
    {
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(strPks.c_str(), ",");
        if ((int)(tSplit.GetFieldCount()) != m_pTable->m_tPriKey.iColumnCounts)
        {
            CHECK_RET(-1, "PK number in the file is wrong.");
        }
        int iNo = 0;
        try
        {
            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                iNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(m_pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    m_pOraQuery->SetParameter(m_pTable->tColumn[iNo].sName, atoi(tSplit[(MDB_UINT32)i]));
                }
                else
                {
                    m_pOraQuery->SetParameter(m_pTable->tColumn[iNo].sName, tSplit[(MDB_UINT32)i]);
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            return iRet;
    }

    /******************************************************************************
    * ��������	:  SetRestoreLocalParam()
    * ��������	:  ���ݶԶ˵Ĳ�ѯָ�룬���ñ��ز�ѯָ��Ĳ���ֵ
    * ����		:  pTable ����Ϣָ��
    * ����		:  pClientQuery �Զ˲�ѯָ��
    * ����		:  pLocalQuery ���ز�ѯָ��
    * ���		:  ��
    * ����ֵ	:  ��-true �� ��-false
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbCheckThread::SetRestoreLocalParam(TMdbClientQuery *pClientQuery, TMdbQuery* pLocalQuery, E_QUERY_TYPE eSqlType)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            for(int j=0; j<m_pTable->iColumnCounts; ++j)
            { 
                if(E_UPDATE == eSqlType && m_pTable->tColumn[j].iDataType == DT_Blob)
                {
                    continue;
                }
                if (pClientQuery->Field(j).isNULL())
                {
                    pLocalQuery->SetParameterNULL(m_pTable->tColumn[j].sName);
                }
                else
                {
                    if(m_pTable->tColumn[j].iDataType == DT_Int)
                    {
                        pLocalQuery->SetParameter(m_pTable->tColumn[j].sName, pClientQuery->Field(j).AsInteger());
                    }
                    else if(m_pTable->tColumn[j].iDataType == DT_DateStamp)
                    {
                        pLocalQuery->SetParameter(m_pTable->tColumn[j].sName, pClientQuery->Field(j).AsDateTimeString());
                    }
                    else
                    {
                        pLocalQuery->SetParameter(m_pTable->tColumn[j].sName, pClientQuery->Field(j).AsString());
                    }
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbCheckThread::SetRestoreLocalParam(TMDBDBQueryInterface *pOraQuery, TMdbQuery* pLocalQuery, E_QUERY_TYPE eSqlType)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            for(int j=0; j<m_pTable->iColumnCounts; ++j)
            { 
                if(E_UPDATE == eSqlType && m_pTable->tColumn[j].iDataType == DT_Blob)
                {
                    continue;
                }
                if (pOraQuery->Field(j).isNULL())
                {
                    pLocalQuery->SetParameterNULL(m_pTable->tColumn[j].sName);
                }
                else
                {
                    if(m_pTable->tColumn[j].iDataType == DT_Int)
                    {
                        pLocalQuery->SetParameter(m_pTable->tColumn[j].sName, pOraQuery->Field(j).AsInteger());
                    }
                    else if(m_pTable->tColumn[j].iDataType == DT_DateStamp)
                    {
                        pLocalQuery->SetParameter(m_pTable->tColumn[j].sName, pOraQuery->Field(j).AsDateTimeString());
                    }
                    else
                    {
                        pLocalQuery->SetParameter(m_pTable->tColumn[j].sName, pOraQuery->Field(j).AsString());
                    }
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbCheckThread::SetRestoreOraParam(TMdbQuery *pLocalQuery, TMDBDBQueryInterface* pOracleQuery, E_QUERY_TYPE eSqlType)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            for(int j=0; j<m_pTable->iColumnCounts; ++j)
            { 
                if(E_UPDATE == eSqlType && m_pTable->tColumn[j].iDataType == DT_Blob)
                {
                    continue;
                }
                if (pLocalQuery->Field(j).isNULL())
                {
                    pOracleQuery->SetParameterNULL(m_pTable->tColumn[j].sName);
                }
                else
                {
                    if(m_pTable->tColumn[j].iDataType == DT_Int)
                    {
                        pOracleQuery->SetParameter(m_pTable->tColumn[j].sName, pLocalQuery->Field(j).AsInteger());
                    }
                    else if(m_pTable->tColumn[j].iDataType == DT_DateStamp)
                    {
                        pOracleQuery->SetParameter(m_pTable->tColumn[j].sName, pLocalQuery->Field(j).AsDateTimeString());
                    }
                    else
                    {
                        pOracleQuery->SetParameter(m_pTable->tColumn[j].sName, pLocalQuery->Field(j).AsString());
                    }
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbCheckThread::SetLocalPKParam(std::string strPks)
    {
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(strPks.c_str(), ",");
        if ((int)(tSplit.GetFieldCount()) != m_pTable->m_tPriKey.iColumnCounts)
        {
            CHECK_RET(-1, "PK number in the file is wrong.");
        }
        int iNo = 0;
        try
        {
            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                iNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(m_pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    m_pLocalQuery->SetParameter(i, atoi(tSplit[(MDB_UINT32)i]));
                }
                else
                {
                    m_pLocalQuery->SetParameter(i, tSplit[(MDB_UINT32)i]);
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            return iRet;
    }

    int TMdbCheckThread::SetDeleteOraParam(TMDBDBQueryInterface* pOraQuery, std::string strPks)
    {
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(strPks.c_str(), ",");
        if ((int)(tSplit.GetFieldCount()) != m_pTable->m_tPriKey.iColumnCounts)
        {
            CHECK_RET(-1, "PK number in the file is wrong.");
        }
        int iNo = 0;
        try
        {
            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                iNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(m_pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    pOraQuery->SetParameter(m_pTable->tColumn[iNo].sName, atoi(tSplit[(MDB_UINT32)i]));
                }
                else
                {
                    pOraQuery->SetParameter(m_pTable->tColumn[iNo].sName, tSplit[(MDB_UINT32)i]);
                }
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION
            return iRet;

    }

    void TMdbCheckThread::SetCompexParam(TMDBDBQueryInterface *pOraQuery)
    {
        //����Ǹ���SQL������Ҫ���ò���
        if(m_pTable->iParameterCount != 0)
        {
            for(int i = 0; i<m_pTable->iParameterCount; i++)
            {
                if(m_pTable->tParameter[i].iDataType == DT_Int && m_pTable->tParameter[i].iParameterType == 0)
                {
                    pOraQuery->SetParameter(m_pTable->tParameter[i].sName,TMdbNtcStrFunc::StrToInt(m_pTable->tParameter[i].sValue));
                }
                else if(m_pTable->tParameter[i].iParameterType == 0)
                {
                    pOraQuery->SetParameter(m_pTable->tParameter[i].sName,m_pTable->tParameter[i].sValue);
                }
            }
        }
    }

    TMdbCheckDataMgr::TMdbCheckDataMgr()
    {
        m_vThreads.clear();
        m_vTables.clear();
        m_vStateTable.clear();
        m_iDealedTable = 0;
        m_pCheckConfig = NULL;
        m_pConfig = NULL;
        m_pDatabase = NULL;
        m_pQuery = NULL;
    }

    TMdbCheckDataMgr::~TMdbCheckDataMgr()
    {
        //�ͷ��߳�ָ��
        std::vector<TMdbCheckThread*>::iterator itor = m_vThreads.begin();
        for (; itor != m_vThreads.end(); ++itor)
        {
            SAFE_DELETE(*itor);
        }
        m_vThreads.clear();

        SAFE_DELETE(m_pDatabase);
        SAFE_DELETE(m_pQuery);
        //SAFE_DELETE(m_pConfig);
    }

bool TMdbCheckDataMgr::m_bDetail = false;

    int TMdbCheckDataMgr::Init(TMdbDataCheckConfig *pConfig)
    {
        int iRet = 0;
        CHECK_OBJ(pConfig);
        m_pCheckConfig = pConfig;

        m_pConfig=TMdbConfigMgr::GetMdbConfig(m_pCheckConfig->m_tCheckInfo.m_strDsn.c_str());
        CHECK_OBJ(m_pConfig);

        m_strPath = m_pCheckConfig->m_tCheckInfo.m_strPath;
        char sCurTime[MAX_TIME_LEN] = {'\0'};
        TMdbDateTime::GetCurrentTimeStr(sCurTime);
        m_strPath.append("/mdbDataCheck_");
        m_strPath.append(sCurTime);
        m_strPath.append("/");
        if (!TMdbNtcDirOper::MakeFullDir(m_strPath.c_str()))
        {
            CHECK_RET(-1, "Make dir[%s] failed.", m_strPath.c_str());
        }

        try
        {
            //���ӱ���mdb
            m_pDatabase = new(std::nothrow) TMdbDatabase();
			if(m_pDatabase == NULL)
			{
				TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new m_pDatabase");
				return ERR_OS_NO_MEMROY;
			}
            if (!m_pDatabase->ConnectAsMgr(m_pCheckConfig->m_tCheckInfo.m_strDsn.c_str()))
            {
                TADD_ERROR(ERROR_UNKNOWN,"Cannot connect to DSN[%s]", m_pCheckConfig->m_tCheckInfo.m_strDsn.c_str());
                return -1;
            }
            m_pQuery = m_pDatabase->CreateDBQuery();
            CHECK_OBJ(m_pQuery);

            //ͳ�Ʊ���Ϣ
            CHECK_RET(StatTables(), "Stat table information failed.");

            //ȷ���Ƚ��߳���
            m_iThreadNum = m_pCheckConfig->m_tCheckInfo.m_iThreadNum;
            if (m_pCheckConfig->m_tCheckInfo.m_iThreadNum > (int)m_vTables.size())
            {
                TADD_NORMAL("Input thread number is larger than the number of tables, will be set to [%d].", m_vTables.size());
                m_iThreadNum = (int)(m_vTables.size());
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION
            return iRet;
    }

    int TMdbCheckDataMgr::Start()
    {
        //(1)�������߳�
        int iRet = 0;
        CHECK_RET(CreateThreads(), "Create threads failed.");   
        //std::vector<TMdbDataCheckTable>::iterator itor = m_vTables.begin();

        //(2)Ϊ�������̣߳���̬�����
        CHECK_RET(CompareTables(), "CompareTables() failed.");

        //(3)�ָ�����
        CHECK_RET(RestoreData(), "RestoreData() failed.");

        //(4)�������߳�
        CHECK_RET(StopThreads(), "StopThreads failed.");

        return iRet;
    }

    int TMdbCheckDataMgr::StatTables()
    {
        int iRet = 0;
        std::vector<TMdbDataCheckTable> vTmpTable;
        TMdbCheckQdgTable *pQdgTable = NULL;
        //ͳ�����б��¼��
        if (m_pCheckConfig->m_tDsn.m_bAll)//�Ƚ����б�
        {
            for(int i=0; i<MAX_TABLE_COUNTS; i++)
            {
                TMdbTable* pTable = m_pConfig->GetTableByPos(i);       

                if(NULL == pTable
                    || TMdbNtcStrFunc::FindString(pTable->sTableName,"DBA_") >= 0 //ϵͳ���Ƚ�
                    || !CheckRepAttr(pTable,pQdgTable))//�����ͬ�����ԣ���oracle�Ƚ�ʱ��ֻ�Ƚ���oracleͬ���ı�
                {
                    continue;	
                }
                CHECK_RET(StatOneTable(pTable, vTmpTable, pQdgTable), "CmpOneTable failed.");//ͳ�Ƶ��ű�
            }  
        }
        else//�Ƚ�ָ���ı�
        {
            std::vector<std::string>::iterator itor = m_pCheckConfig->m_tDsn.m_vTables.begin();
            for (; itor != m_pCheckConfig->m_tDsn.m_vTables.end(); ++itor)
            {
                TMdbTable * pTable = m_pConfig->GetTableByName((*itor).c_str());
                if(NULL == pTable)
                {
                    TADD_WARNING("Invalid table name[%s], not to be compared.", (*itor).c_str());
                    continue;
                }
                if(TMdbNtcStrFunc::FindString(pTable->sTableName,"DBA_") >= 0 
                    || !CheckRepAttr(pTable, pQdgTable))//�����ͬ�����ԣ���oracle�Ƚ�ʱ��ֻ�Ƚ���oracleͬ���ı�
                {
                    TADD_WARNING("Table [%s] will not to compare", (*itor).c_str());
                    continue;	
                }
                CHECK_RET(StatOneTable(pTable, vTmpTable, pQdgTable), "CmpOneTable failed.");//ͳ�Ƶ��ű�
            }
        }
        //������¼������,������������б������¼���Ӵ�С���У�
        CHECK_RET(GenTableList(vTmpTable), "GenTableList failed.");

        return iRet;
    }

    int TMdbCheckDataMgr::StatOneTable(TMdbTable * pTable, std::vector<TMdbDataCheckTable>& vTmpTable, TMdbCheckQdgTable* pQdg)
    {
        int iRet = 0;
        std::string strTmpSql;
        TMdbDataCheckTable tTmpTable;
        std::string strWhere;
        if (!m_pCheckConfig->m_tDsn.m_strFilterSql.empty())
        {
            strWhere.append(" where ");
            strWhere.append(m_pCheckConfig->m_tDsn.m_strFilterSql);
        }
        strTmpSql.clear();
        if (strWhere.empty())
        {
            strTmpSql = "select real_counts NUM from dba_tables where table_name = '";
            strTmpSql.append(pTable->sTableName);
            strTmpSql.append("'");
        }
        else
        {
            strTmpSql = "select count(*) NUM from ";
            strTmpSql.append(pTable->sTableName);
            strTmpSql.append(strWhere);
        }

        tTmpTable.Clear();
        tTmpTable.m_bDealed = false;
        //tTmpTable.m_iTableID = pTable->iTableID;
        tTmpTable.m_strTableName = pTable->sTableName;
        if (pQdg!=NULL)
        {
            tTmpTable.m_bIsQdgTable = true;
            tTmpTable.m_strLoadSql = pQdg->m_strLoadSql;
        }
       
        try
        {
            m_pQuery->Close();
            m_pQuery->SetSQL(strTmpSql.c_str());
            m_pQuery->Open();
            if (m_pQuery->Next())
            {
                tTmpTable.m_iRecordNum = (int)(m_pQuery->Field("NUM").AsInteger());
            }
        }
        CATCH_MDB_DATA_CHECK_EXEPTION

            for (int i = 0; i< pTable->m_tPriKey.iColumnCounts; i++)
            {
                tTmpTable.m_iPkLen += pTable->tColumn[pTable->m_tPriKey.iColumnNo[i]].iColumnLen;
            }

            vTmpTable.push_back(tTmpTable);
            return iRet;
    }

    bool TMdbCheckDataMgr::CheckRepAttr(TMdbTable * pTable, TMdbCheckQdgTable* &pQdg)
    {
        pQdg = NULL;
        switch(m_pCheckConfig->m_tDsn.m_eType)
        {
        case E_DSN_ORACLE://��oracle֮�Ƚ���֮ͬ���ı�
            {
                if (pTable->iRepAttr == REP_NO_REP)//�Ƿ���qdg��
                {
                    for (long unsigned int j = 0; j<m_pCheckConfig->m_tDsn.m_vQdgTable.size(); j++)
                    {
                        if (TMdbNtcStrFunc::StrNoCaseCmp(m_pCheckConfig->m_tDsn.m_vQdgTable[j].m_strTableName.c_str(), pTable->sTableName) == 0)
                        {
                            pQdg = &m_pCheckConfig->m_tDsn.m_vQdgTable[j];
                            return true;
                        }
                    }
                }
            else if (pTable->iRepAttr  == REP_FROM_DB)//��oracleͬ���ı�Ҳ���Ա�����qdg��������
            {
                for (long unsigned int j = 0; j<m_pCheckConfig->m_tDsn.m_vQdgTable.size(); j++)
                {
                    if (TMdbNtcStrFunc::StrNoCaseCmp(m_pCheckConfig->m_tDsn.m_vQdgTable[j].m_strTableName.c_str(), pTable->sTableName) == 0)
                    {
                        pQdg = &m_pCheckConfig->m_tDsn.m_vQdgTable[j];
                    }
                }

                return true;
            }
            else if(pTable->iRepAttr == REP_TO_DB)
                {
                    return true;
                }
                break;
            }
        case E_DSN_MDB://��MDB�Ƚ����б�
            {
                return true;
            }
        case  E_DSN_UNKOWN:
            {
                TADD_ERROR(ERROR_UNKNOWN,"Unknown DSN type.");
                break;
            }
        }
        return false;
    }

    TMdbDataCheckTable* TMdbCheckDataMgr::GetCmpTable(std::string strTableName)
    {
        std::vector<TMdbDataCheckTable>::iterator itor = m_vTables.begin();
        for (; itor!=m_vTables.end(); ++itor)
        {
            if (TMdbNtcStrFunc::StrNoCaseCmp((*itor).m_strTableName.c_str(), strTableName.c_str()) == 0)
            {
                return &(*itor);
            }
        }
        return NULL;
    }

    int TMdbCheckDataMgr::GenTableList(std::vector<TMdbDataCheckTable>& vTmpTable)
    {
        int iRet = 0;
        std::vector<TMdbDataCheckTable>::iterator itormax;
        std::vector<TMdbDataCheckTable>::iterator itor;
        while(vTmpTable.size() > 0)
        {
            itor = vTmpTable.begin();
            itormax = itor;
            for (; itor!=vTmpTable.end(); ++itor)
            {
                if ((*itormax).m_iRecordNum < (*itor).m_iRecordNum)
                {
                    itormax =itor;
                }
            }
            m_vTables.push_back((*itormax));
            vTmpTable.erase(itormax);
        }

        return iRet;
    }
    int TMdbCheckDataMgr::CreateThreads()
    {
        int iRet = 0;
        for (int i = 0; i<m_iThreadNum; i++)
        {
            TMdbCheckThread *pThread = new(std::nothrow) TMdbCheckThread();
            CHECK_OBJ(pThread);
            CHECK_RET(pThread->Init(m_pCheckConfig,m_pConfig, m_strPath), "Thread init failed.");
            pThread->m_iThreadNO = i;
            CHECK_RET(pThread->Start(), "Thread start failed.");
            m_vThreads.push_back(pThread);
        }

        return iRet;
    }

    int TMdbCheckDataMgr::CompareTables()
    {
        int iRet = 0;
        TADD_NORMAL("Begin to Compare ...");
        while(1)
        {
            //����δ����ı�Ϊ�����̷߳����
            if(m_iDealedTable < (int)m_vTables.size())
            {
                for (int i = 0; i<m_iThreadNum; i++)
                {
                    if ((E_CHECK_THREAD_FREE == m_vThreads[i]->m_eState || E_CHECK_THREAD_ERROR == m_vThreads[i]->m_eState )\
                        && m_iDealedTable<(int)m_vTables.size())
                    {
                        m_vThreads[i]->SetTableToCmp(&m_vTables[m_iDealedTable]);
                        m_iDealedTable++;
                    }
                }
            }
            //�������ϣ�����Ƿ������̴߳������
            if (m_iDealedTable == (int)m_vTables.size())//���Ѿ��������
            {
                int j = 0;
                for (; j<m_iThreadNum; j++)
                {
                    if (E_CHECK_THREAD_FREE  != m_vThreads[j]->m_eState && E_CHECK_THREAD_ERROR != m_vThreads[j]->m_eState)
                    {
                        break;
                    }
                }
                if (j == m_iThreadNum)//�����̶߳��������
                {
                    break;
                }
            }
            TMdbDateTime::MSleep(100);
        } 
        return iRet;
    }

    int TMdbCheckDataMgr::RestoreData()
    {
        int iRet = 0;
        TMdbFileList tFileList;
        tFileList.Init(m_strPath.c_str());
        tFileList.GetFileList("DIFF_");
        int iDiffTable = tFileList.GetFileCounts();
        if (iDiffTable==0)
        {
            return iRet;
        }
        TADD_NORMAL("There are %d tables different from peer, will you restore the data[Y/N]?", iDiffTable);
        char sTemp[4];
        memset(sTemp,0,sizeof(sTemp));
        scanf("%3s",sTemp);
        TMdbNtcStrFunc::Trim(sTemp);
        if (TMdbNtcStrFunc::StrNoCaseCmp(sTemp,"N") == 0)
        {
            return iRet;
        }

        //�����
        TADD_NORMAL("Begin to Restore ...");
        char sFileName[MAX_FILE_NAME];
        bool bDealed = false;
        const char* psRealName = NULL;
        for (int n= 0; n<iDiffTable; n++)
        {
            bDealed = false;
            tFileList.Next(sFileName);
            while(!bDealed)
            {
                for (int i = 0; i<m_iThreadNum; i++)
                {
                    if (E_CHECK_THREAD_FREE == m_vThreads[i]->m_eState || E_CHECK_THREAD_ERROR == m_vThreads[i]->m_eState)
                    {
                        psRealName = TMdbNtcFileOper::GetFileName(sFileName);
                        m_vThreads[i]->SetTableToRestore(GetCmpTable(&psRealName[strlen("DIFF_")]));
                        
                        bDealed = true;
                        break;
                    }
                }
                if (!bDealed)
                {
                    TMdbDateTime::MSleep(100);
                }
            }
        }

        //�������ϣ�����Ƿ������̴߳������
        while(1)
        {
            int j = 0;
            for (; j<m_iThreadNum; j++)
            {
                if (E_CHECK_THREAD_FREE != m_vThreads[j]->m_eState && E_CHECK_THREAD_ERROR != m_vThreads[j]->m_eState)
                {
                    break;
                }
            }
            if (j == m_iThreadNum)//�����̶߳��������
            {
                break;
            }
            TMdbDateTime::MSleep(100);
        }
        return iRet;
    }

    int TMdbCheckDataMgr::StopThreads()
    {
        int iRet = 0;

        TMdbCheckThread* pThread = NULL; 
        std::vector<TMdbCheckThread*>::iterator itor = m_vThreads.begin();
        for(; itor != m_vThreads.end(); itor++)
        {
            pThread = *(itor);
            if(NULL == pThread)
            {
                continue;
            }
            pThread->Stop();
        }

        TMdbDateTime::MSleep(1000);
        return iRet;
    }

//}
