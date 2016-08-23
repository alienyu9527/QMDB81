#include "Tools/mdbOverdueDataCtrl.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


    int TCleanFileParser::ParseLine(char* sLineText)
    {
        TADD_DETAIL("LineText:[%s]", sLineText);

        TTableCleanInfo tTab;
        //把当前文字解析为TTableCleanInfo

        int iPos = 0;
        int iLen = strlen(sLineText);

        char sTabName[MAX_NAME_LEN] = {0};
        char sDsnName[MAX_NAME_LEN] = {0};
        char sTmpSql[MAX_SQL_LEN] ={0};

        
        // tablename
        bool bTabFound = false;
        while(iPos < iLen)
        {
            if(sLineText[iPos] == '@')
            {
                bTabFound = true;
                memset(sTabName, 0, sizeof(sTabName));
                strncpy(sTabName, sLineText, iPos);
                tTab.m_sTableName = sTabName;
                TADD_DETAIL("Table_name=[%s], iPos=%d", sTabName, iPos);
                break;
            }
            else
            {
                iPos++;
                continue;
            }
        }
        if(false == bTabFound)
        {
            TADD_ERROR(-1,"no tablename found,invalid line text:[%s]", sLineText);
            return -1;
        }

        // @
        iPos++;

        // dsnname
        int iDsnBeginPos = iPos;
        bool bDsnFound = false;
        while(iPos < iLen)
        {
            if(sLineText[iPos] == '=')
            {
                bDsnFound  = true;
                memset(sDsnName, 0, sizeof(sDsnName));
                strncpy(sDsnName, sLineText + iDsnBeginPos, iPos-iDsnBeginPos);
                tTab.m_sDsn = sDsnName;
                TADD_DETAIL("Dsn_name=[%s], iPos=%d, iBeginPos=%d", sDsnName, iPos , iDsnBeginPos);
                break;
            }
            else
            {
                iPos++;
                continue;
            }
        }
        if(false == bDsnFound)
        {
            TADD_ERROR(-1,"no dsnname found,invalid line text:[%s]", sLineText);
            return -1;
        }

        // = 
        iPos++;

        //[SQL]
        bool bSqlFound = false;
        memset(sTmpSql, 0, sizeof(sTmpSql));
        int iSqlBeginPos = 0;
        while(iPos < iLen)
        {
            if(sLineText[iPos] != '[' && sLineText[iPos] != ']')
            {
                iPos++;
                continue;
            }

            if(sLineText[iPos] == '[')
            {
                iPos++;
                iSqlBeginPos = iPos;
                continue;
            }

            if(sLineText[iPos] == ']')
            {
                strncpy(sTmpSql, sLineText+ iSqlBeginPos,iPos - iSqlBeginPos );
                bSqlFound = true;
                tTab.m_sSQL = sTmpSql;
                TADD_DETAIL("sql=[%s], iPos=%d, iBeginPos=%d", sTmpSql, iPos , iSqlBeginPos);
                break;
            }
            
        }
        if(false == bSqlFound)
        {
            TADD_ERROR(-1,"no ql found,invalid line text:[%s]", sLineText);
            return -1;
        }
        
        //把TTableCleanInfo 项保存
        m_vCleanTab.push_back(tTab);

        return 0;
    }

    int TCleanFileParser::Parse(char* sFullFileName)
    {
        int iRet = 0;
        CHECK_OBJ(sFullFileName);

        if(false == TMdbNtcFileOper::IsExist(sFullFileName))
        {
            TADD_ERROR(-1,"config file[%s] not exist.",sFullFileName);
            return -1;
        }

        FILE * fp = fopen(sFullFileName, "r");
        if(NULL == fp)
        {
            CHECK_RET(-1,"Can't open file=[%s],errno=%d,errmsg=%s.",sFullFileName,errno,strerror(errno));
        }
        
        char sLineText[MAX_SQL_LEN] = {0};
            
        fseek(fp,0,SEEK_SET);
        //遍历文件
        while(fgets(sLineText, sizeof(sLineText),fp))
        {
            TMdbNtcStrFunc::Trim(sLineText);

            //空行直接过滤
            if ( ( sLineText[0] == '\n' ) || strlen(sLineText) == 0) 
                continue;
            
           // 注释行跳过
            if(sLineText[0] == '#' )
            {
                continue;
            }

            if(ParseLine(sLineText) < 0)
            {
                TADD_ERROR(-1,"Line parse failed.[%s]", sLineText);
            }

            memset(sLineText, 0, sizeof(sLineText));
        }
        
        SAFE_CLOSE(fp);    
        return iRet;
    }

    TMdbConnSet::TMdbConnSet()
    {
        m_pDBLink = NULL;
        m_pQuery = NULL;
    }


    TMdbConnSet::~TMdbConnSet()
    {
        SAFE_DELETE(m_pQuery);
        SAFE_DELETE(m_pDBLink);  
    }

    int TMdbConnSet::Create(const char* psDsnName)
    {
        int iRet = 0;
        CHECK_OBJ(psDsnName);

        m_pDBLink = new(std::nothrow) TMdbDatabase();
        CHECK_OBJ(m_pDBLink);

        m_sDsn = psDsnName;

        try
        {
            if(m_pDBLink->ConnectAsMgr(psDsnName) == false)
            {
                m_pDBLink->Disconnect();
                SAFE_DELETE(m_pDBLink);
                CHECK_RET(ERR_DB_NOT_CONNECTED,"connect(%s) fail",psDsnName);
            }

            m_pQuery = m_pDBLink->CreateDBQuery();
            CHECK_OBJ(m_pQuery);
        }
        catch(TMdbException& e)
        {
            m_pDBLink->Disconnect();
            SAFE_DELETE(m_pDBLink);
            CHECK_RET(ERR_DB_NOT_CONNECTED,"connect(%s)ERROR_SQL=%s.\nERROR_MSG=%s\n",
                m_sDsn.c_str(), e.GetErrSql(), e.GetErrMsg());
        }

        return iRet;
    }

    int TMdbConnSet::Destroy()
    {
        m_pQuery->Close();
        m_pDBLink->Disconnect();
        SAFE_DELETE(m_pQuery);
        SAFE_DELETE(m_pDBLink);    
        return 0;
    }

    TMdbQuery* TMdbConnSet::GetQuery()
    {
        return m_pQuery;
    }

    TColmItem::TColmItem()
    {
        Clear();
    }

    TColmItem::~TColmItem()
    {
    }

    void TColmItem::Clear()
    {
        m_bIsTime= false;
        m_bNull = false;
        memset(m_sName, 0, sizeof(m_sName));
        memset(m_sValue, 0, sizeof(m_sValue));
    }

    TRecordParser::TRecordParser()
    {
        m_iRecdLen = 0;
    }

    TRecordParser::~TRecordParser()
    {
    }

    void TRecordParser::Clear()
    {
        m_iRecdLen = 0;
        m_vColmSet.clear();
    }

    void TRecordParser::Print()
    {
        TADD_DETAIL("Columns as follow:");
        std::vector<TColmItem>::iterator itor = m_vColmSet.begin();
        for(; itor != m_vColmSet.end(); ++itor)
        {
            TADD_DETAIL("[%s]=[%s]", itor->m_sName, itor->m_sValue);
        }
    }


    int TRecordParser::Parse(char* pszRecord)
    {
        int iRet = 0;
        if(NULL == pszRecord )
        {
            iRet = ERR_APP_INVALID_PARAM;
            TADD_ERROR(-1,"NULL = =pszRecord.");
            return iRet;
        }

        Clear();

        int iPos = 0;

        // ["^^"] head,2Byte
        if('^' != pszRecord[iPos] || '^' != pszRecord[iPos+1])
        {
            TADD_ERROR(-1,"record not begin with ^^");
            iRet = -1;
            TADD_FUNC("Finish.");
            return iRet;
        }
        iPos += 2;

        // [length] 4Byte
        int iLen = 0;
        iLen += (*(pszRecord + iPos + 0) - '0') * 1000;
        iLen += (*(pszRecord + iPos + 1) - '0') * 100;
        iLen += (*(pszRecord + iPos + 2) - '0') * 10;
        iLen += (*(pszRecord + iPos + 3) - '0') ;
        if(iLen <= 0 || iLen > MAX_VALUE_LEN || '&' != pszRecord[iLen - 2] || '&' != pszRecord[iLen - 1])
        {
            TADD_ERROR(-1,"parse len(%d) invalid", iLen);
            TADD_FUNC("Finish.");
            return -1;
        }
        m_iRecdLen = iLen;
        iPos += 4;

       
        // [,@ColmName=ColmValue ...] x Byte
        int iColmLen =0;
        iRet = GetColm(pszRecord + iPos, iColmLen);
        if(iRet < 0)
        {
            TADD_ERROR(-1,"parse column failed");
            TADD_FUNC("Finish.");
            return -1;
        }
        iPos += iColmLen;

        // ["&&"] 2Byte
        // end,do nothing

        m_sRecd = pszRecord;
          
        TADD_FUNC("Finish.");
        return iRet;
    }

    const char* TRecordParser::GetRecordStr()
    {
        return m_sRecd.c_str();
    }


    int TRecordParser::GetColm(char* psData, int& iColmLen)
    {
        int iRet = 0;
        int iPos = 0;
        int iOneColmLen = 0;
        m_vColmSet.clear();
        
        while('\0' != psData[iPos] &&  '|' != psData[iPos] && '&' != psData[iPos] )
        {
            TColmItem tColmItem;

            if(0 != GetColumnValue(psData + iPos, tColmItem, iOneColmLen))
            {
                TADD_ERROR(-1,"get column value failed.");
                iRet = -1;
                break;
            }
            m_vColmSet.push_back(tColmItem);
            iPos += iOneColmLen;
        }
        
        iColmLen = iPos;
        TADD_DETAIL("iColmLen = %d", iColmLen);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TRecordParser::GetColumnValue(char* pValuePair,TColmItem & stColumnValue,int &iValuePairLen)
    {
        int iRet = 0;
        int iPos = 0;

        // columnName
        if(',' != pValuePair[iPos] || '@' != pValuePair[iPos+1])
        {

            TADD_ERROR(-1,"column not begin with ',@'");
            iRet = -1;
            TADD_FUNC("Finish.");
            return iRet;
        }
        iPos += 2;
        int iNameBegin = iPos;
        
        while('\0' != pValuePair[iPos] &&  '|' != pValuePair[iPos] && '&' != pValuePair[iPos] )
        {
            if('=' == pValuePair[iPos])
            {
                break;
            }
            ++iPos;
        }

        int iNameEnd = iPos;
        int iNameLen = iNameEnd - iNameBegin;

        if(0 == iNameLen)
        {
            TADD_ERROR(-1,"not valid column name.");
            iRet = -1;
            TADD_FUNC("Finish.");
            return iRet;
        }

        strncpy(stColumnValue.m_sName, pValuePair + iNameBegin, iNameLen);
        stColumnValue.m_sName[iNameLen] = 0;


        // "="
        iPos++;
        
        int iValueBegin = iPos;

        // columnValue
        while((',' != pValuePair[iPos] || '@' != pValuePair[iPos + 1]) && '\0' != pValuePair[iPos] &&   '&' != pValuePair[iPos] )
        {
            ++iPos;
        }
        int iValueEnd = iPos;
        int iValueLen = iValueEnd - iValueBegin;

        strncpy(stColumnValue.m_sValue,pValuePair+iValueBegin, iValueLen);
        stColumnValue.m_sValue[iValueLen] = 0;

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(stColumnValue.m_sValue, "(nil)"))
        {
            stColumnValue.m_bNull = true;
        }

        iValuePairLen = iPos;
        TADD_DETAIL("get column pair:[%s=%s]", stColumnValue.m_sName, stColumnValue.m_sValue);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    TRecordWriter::TRecordWriter()
    {
        m_fp = NULL;
        Clear();
    }

    TRecordWriter::~TRecordWriter()
    {
        SAFE_CLOSE(m_fp);
    }

    int TRecordWriter::Init(const char* sFilePath, char* sFileName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        CHECK_OBJ(sFileName);
        CHECK_OBJ(sFilePath);

        Clear();
        
        bool bExist = TMdbNtcDirOper::IsExist(sFilePath);
        if(bExist == false)
        {
            if(TMdbNtcDirOper::MakeFullDir(sFilePath) == false)
            {
                TADD_ERROR(-1,"Can't create dir=[%s].",sFilePath);
                return -1;
            }
            else
            {
                TADD_NORMAL("Create path:[%s]", sFilePath);
            }
        }

        snprintf(m_sFileName, sizeof(m_sFileName),"%s%s", sFilePath, sFileName);

        TADD_FLOW("File:[%s]", m_sFileName);
        
        m_fp =  fopen(m_sFileName,"wb");
        CHECK_OBJ(m_fp);
        setbuf(m_fp,NULL);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TRecordWriter::Write(char* sData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iLen = strlen(sData);
        if(m_iBufPos + iLen < MAX_SEND_BUF)
        {
            strncpy(&m_sBuf[m_iBufPos], sData, iLen);
            m_iBufPos+=iLen;
        }
        else
        {
            if(fwrite(m_sBuf, m_iBufPos, 1, m_fp) == 0)
            {
                TADD_ERROR(-1,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));
                return -1;
            }
            m_iFileSize+=m_iBufPos;
            strncpy(m_sBuf, sData, iLen);
            m_iBufPos = iLen;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TRecordWriter::Write()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_iBufPos <= 0){return iRet;}
        if(fwrite(m_sBuf, m_iBufPos, 1, m_fp) == 0)
        {
            TADD_ERROR(-1,"fwrite() failed, errno=%d, errmsg=[%s].",errno, strerror(errno));
            return -1;
        }
        m_iFileSize+=m_iBufPos;
        m_iBufPos = 0;
        TADD_FUNC("Finish.");
        return iRet;
    }

    void TRecordWriter::Clear()
    {
        SAFE_CLOSE(m_fp);
        memset(m_sFileName, 0, MAX_PATH_NAME_LEN);
        memset(m_sBuf, 0, sizeof(m_sBuf));
        m_iBufPos = 0;
        m_iFileSize = 0;
        
    }

    TOverDueDataCtrl::TOverDueDataCtrl()
    {
        m_pConfigOper = NULL;
        m_bAllTab = false;
        m_pCurTabQry = NULL;
        m_pCurTabWriter = NULL;
        m_pFilePrc = NULL; 
    }

    TOverDueDataCtrl::~TOverDueDataCtrl()
    {
        SAFE_DELETE(m_pConfigOper);
        SAFE_DELETE(m_pCurTabWriter);
        SAFE_DELETE(m_pFilePrc);

        std::vector<TMdbConnSet*>::iterator itor = m_vMdbConn.begin();
        for(; itor != m_vMdbConn.end(); ++itor)
        {
            TMdbConnSet* pConn = *itor;
            if(NULL != pConn)
            {
                pConn->Destroy();
                SAFE_DELETE(pConn);
            }        
        }
    }

    int TOverDueDataCtrl::RunBak(const char* sPath, char* sFileName)
    {
        int iRet = 0;
        TADD_FUNC("Start");

        CHECK_OBJ(sPath);
        CHECK_OBJ(sFileName);

        if(false == TMdbNtcFileOper::IsExist(sFileName) )
        {
            TADD_ERROR(-1,"clean config File[%s] not exist!", sFileName);
            iRet = -1;
            return iRet;
        }

        TADD_NORMAL("Clean Config File:[%s]", sFileName);

        m_sBakPath = sPath;
        if(m_sBakPath[m_sBakPath.size() - 1] != '/')
        {
            m_sBakPath.append("/");
        }
        
        if(false == TMdbNtcDirOper::IsExist(m_sBakPath.c_str()) )
        {
            if(false == TMdbNtcDirOper::MakeFullDir(m_sBakPath.c_str()))
            {
                TADD_ERROR(-1,"path[%s] not exist, create it failed.", m_sBakPath.c_str());
                iRet = -1;
                return iRet;
            }
            TADD_NORMAL("path[%s] not exist, create it!",m_sBakPath.c_str());
        }

        TADD_NORMAL("Bak File Path:[%s]", m_sBakPath.c_str());

        m_pConfigOper = new(std::nothrow) TCleanFileParser();
        CHECK_OBJ(m_pConfigOper);

        if(m_pConfigOper->Parse(sFileName)<0)
        {
            TADD_ERROR(-1,"parse config file[%s] failed.", sFileName);
            iRet = -1;
            return iRet;
        }

        if(CheckCleanConfig() < 0)
        {
            TADD_ERROR(-1,"clean config file is not valid.");
            iRet = -1;
            return iRet;
        }

        TADD_NORMAL("Bak Process begin.");

        std::vector<TTableCleanInfo>::iterator itor = m_pConfigOper->m_vCleanTab.begin();
        for(; itor != m_pConfigOper->m_vCleanTab.end();++itor)
        {
            TTableCleanInfo& tTabInfo = *itor;
            iRet = BakTable(tTabInfo);
            if(iRet < 0)
            {
                TADD_ERROR(-1,"Bak Table[%s] ,Dsn[%s] failed.", tTabInfo.m_sTableName.c_str(), tTabInfo.m_sDsn.c_str());
                iRet = -1;
                return iRet;
            }
        }

        TADD_NORMAL("Bak Process finished.");
        
        TADD_FUNC("Finish");
        return iRet;
    }

    int TOverDueDataCtrl::RunClean(const char* sPath, char* sTabName, char* sDsnName)
    {
        int iRet = 0;
        TADD_FUNC("Start");

        CHECK_OBJ(sPath);
        CHECK_OBJ(sTabName);

        if(false == TMdbNtcDirOper::IsExist(sPath) )
        {
            TADD_ERROR(-1,"bak Path[%s] not exist!", sPath);
            iRet = -1;
            return iRet;
        }

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sTabName, "all"))
        {
            TADD_NORMAL("Clean All config table");
            m_bAllTab = true;
        }

        if(false == m_bAllTab && NULL == sDsnName)
        {
            TADD_ERROR(-1,"Clean one table need dsn name.");
            return -1;
        }

        m_sBakPath = sPath;
        if(m_sBakPath[m_sBakPath.size() - 1] != '/')
        {
            m_sBakPath.append("/");
        }
        
        if(false == TMdbNtcDirOper::IsExist(m_sBakPath.c_str()) )
        {
            TADD_ERROR(-1,"Bak file path[%s] not exist.",m_sBakPath.c_str());
            return -1;
        }

        TADD_NORMAL("Clean Table:[%s], bak file path:[%s]", sTabName, sPath);


        if(m_bAllTab)
        {   
            iRet = CleanAllTable();
        }
        else
        {
            char sFileHeader[MAX_FILE_NAME] = {0};
            char sUpperTabName[MAX_NAME_LEN] ={0};
            char sUpperDsnName[MAX_NAME_LEN]={0};

            SAFESTRCPY(sUpperTabName, sizeof(sUpperTabName), sTabName);
            SAFESTRCPY(sUpperDsnName, sizeof(sUpperDsnName), sDsnName);

			//mjx sql tool add start
			TMdbNtcStrFunc::ToUpper(sUpperTabName);
			TMdbNtcStrFunc::ToUpper(sUpperDsnName);
			//mjx sql tool add end
            snprintf(sFileHeader, sizeof(sFileHeader), "BAK.%s.%s", sUpperDsnName,sUpperTabName);

            m_tFileList.Clear();
            CHECK_RET(m_tFileList.Init(m_sBakPath.c_str()),"Init bak path file list handler failed.");
            CHECK_RET(m_tFileList.GetFileList(sFileHeader, ".OK"),"Read File List failed.");

            if(m_tFileList.GetFileCounts() <= 0)
            {
                TADD_ERROR(-1,"no bak file for[DSN:%s; TABLE:%s]", sUpperDsnName, sUpperTabName);
                return -1;
            }

            
            char sFileName[MAX_FILE_NAME] ={0};
            memset(sFileName, 0, sizeof(sFileName));
            while(0 == m_tFileList.Next(sFileName) )
            {

                TADD_NORMAL("Process File:[%s]", sFileName);
                
                if(CleanOneTable(sUpperTabName, sUpperDsnName, sFileName) < 0)
                {
                    TADD_ERROR(-1,"Clean table[%s], Dsn[%s] ovedue data failed..", sUpperTabName,sUpperDsnName);
                    continue;
                }     

                memset(sFileName, 0, sizeof(sFileName));
            }
        
        }
        
        TADD_FUNC("Finish");
        return iRet;
    }


    int TOverDueDataCtrl::RunRestore(const char* sPath, char* sTabName, char* sDsnName)
    {
        int iRet = 0;
        TADD_FUNC("Start");

        CHECK_OBJ(sPath);
        CHECK_OBJ(sTabName);

        if(false == TMdbNtcDirOper::IsExist(sPath) )
        {
            TADD_ERROR(-1,"bak Path[%s] not exist!", sPath);
            iRet = -1;
            return iRet;
        }

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sTabName, "all"))
        {
            TADD_NORMAL("Restore All config table");
            m_bAllTab = true;
        }

        if(false == m_bAllTab && NULL == sDsnName)
        {
            TADD_ERROR(-1,"Restore one table need dsn name.");
            return -1;
        }

        m_sBakPath = sPath;
        if(m_sBakPath[m_sBakPath.size() - 1] != '/')
        {
            m_sBakPath.append("/");
        }
        
        if(false == TMdbNtcDirOper::IsExist(m_sBakPath.c_str()) )
        {
            TADD_ERROR(-1,"Bak file path[%s] not exist.",m_sBakPath.c_str());
            return -1;
        }

        TADD_NORMAL("Restore Table:[%s], bak file path:[%s]", sTabName, sPath);


        if(m_bAllTab)
        {   
            iRet = RestoreAllTable();
        }
        else
        {
            char sFileHeader[MAX_FILE_NAME] = {0};
            char sUpperTabName[MAX_NAME_LEN] ={0};
            char sUpperDsnName[MAX_NAME_LEN]={0};

            SAFESTRCPY(sUpperTabName, sizeof(sUpperTabName), sTabName);
            SAFESTRCPY(sUpperDsnName, sizeof(sUpperDsnName), sDsnName);
            //mjx sql tool add start
			TMdbNtcStrFunc::ToUpper(sUpperTabName);
			TMdbNtcStrFunc::ToUpper(sUpperDsnName);
			//mjx sql tool add end
            snprintf(sFileHeader, sizeof(sFileHeader), "BAK.%s.%s", sUpperDsnName,sUpperTabName);

            m_tFileList.Clear();
            CHECK_RET(m_tFileList.Init(m_sBakPath.c_str()),"Init bak path file list handler failed.");
            CHECK_RET(m_tFileList.GetFileList(sFileHeader, ".OK"),"Read File List failed.");

            if(m_tFileList.GetFileCounts() <= 0)
            {
                TADD_ERROR(-1,"no bak file for[DSN:%s; TABLE:%s]", sUpperDsnName, sUpperTabName);
                return -1;
            }

            
            char sFileName[MAX_FILE_NAME] ={0};
            memset(sFileName, 0, sizeof(sFileName));
            while(0 == m_tFileList.Next(sFileName ) )
            {

                TADD_NORMAL("Process File:[%s]", sFileName);
                
                if(RestoreOneTable(sUpperTabName, sUpperDsnName, sFileName) < 0)
                {
                    TADD_ERROR(-1,"Restore table[%s], Dsn[%s] ovedue data failed..", sUpperTabName,sUpperDsnName);
                    continue;
                }     

                memset(sFileName, 0, sizeof(sFileName));
            }
        
        }
        
        TADD_FUNC("Finish");
        return iRet;
    }

    int TOverDueDataCtrl::CheckCleanConfig()
    {

        std::vector<TTableCleanInfo>::iterator itor = m_pConfigOper->m_vCleanTab.begin();
        for(; itor != m_pConfigOper->m_vCleanTab.end();++itor)
        {
            TTableCleanInfo& tTabInfo = *itor;
            TMdbQuery* pQuery = GetMdbQuery(tTabInfo.m_sDsn.c_str());
            if(NULL == pQuery)
            {
                TADD_ERROR(-1,"can't connect qmdb.dsn=[%s]", tTabInfo.m_sDsn.c_str());
                return -1;
            }

            try
            {
                pQuery->Close();
                pQuery->SetSQL(tTabInfo.m_sSQL.c_str());
            }
            catch(TMdbException &oe)
            {
                TADD_ERROR(-1,"Err-msg=[%s], sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
                return -1;
            }
            catch(...)
            {
                TADD_ERROR(-1,"unkown exception");
                return -1;
            }
        }

        return 0;
    }

    int TOverDueDataCtrl::GetTableInfoFromFileName(const char* psFileName, char* psDsnName, char* psTabName)
    {
        int iRet = 0;
        
        CHECK_OBJ(psFileName);
        CHECK_OBJ(psDsnName);
        CHECK_OBJ(psTabName);

        char sFileName[MAX_FILE_NAME] = {0};
        SAFESTRCPY(sFileName, sizeof(sFileName), TMdbNtcFileOper::GetFileName(psFileName));
        TADD_DETAIL("sFileName=[%s]",sFileName);

        TMdbNtcSplit tSplit;
        tSplit.SplitString(sFileName,'.');
        if(tSplit.GetFieldCount() < 4)
        {
            TADD_ERROR(-1,"invliad bak file name:[%s]", psFileName);
            return -1;
        }

        strncpy(psDsnName, tSplit[1],MAX_NAME_LEN-1);
        strncpy(psTabName, tSplit[2],MAX_NAME_LEN-1 );
        
        return iRet;
    }


    TMdbQuery* TOverDueDataCtrl::GetMdbQuery(const char* sDsnName)
    {
        if(NULL == sDsnName)
        {
            TADD_ERROR(-1,"Dsn name is null.");
            return NULL;
        }
        
        std::vector<TMdbConnSet*>::iterator itor = m_vMdbConn.begin();
        for( ; itor != m_vMdbConn.end(); ++itor)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sDsnName, (*itor)->m_sDsn.c_str()))
            {
                return (*itor)->GetQuery();
            }
        }

        TMdbConnSet* pMdbConn = new(std::nothrow) TMdbConnSet();
        if(NULL == pMdbConn)
        {
            TADD_ERROR(-1,"new TMdbConnSet failed.");
            return NULL;
        }

        if(pMdbConn->Create(sDsnName) < 0)
        {
            TADD_ERROR(-1,"create mdb connect failed.");
            return NULL;
        }

        m_vMdbConn.push_back(pMdbConn);

        return pMdbConn->GetQuery(); 

        
    }


    int TOverDueDataCtrl::BakTable(const TTableCleanInfo& tTabInfo)
    {
        int iRet = 0;
        TADD_NORMAL("Bak Table[%s] begin...", tTabInfo.m_sTableName.c_str());
        m_pCurTabQry = GetMdbQuery(tTabInfo.m_sDsn.c_str());
        if(NULL == m_pCurTabQry)
        {   
            TADD_ERROR(-1,"get query failed.");
            return -1;
        }

        MDB_INT64 iBakCount = 0;
        int iFieldCnt = 0;

        try
        {
            iRet = InitTabWriter(tTabInfo);
            if(iRet < 0)
            {
                TADD_ERROR(-1,"init bak file write handler failed.");
                return iRet;
            }
            m_pCurTabQry->Close();
            m_pCurTabQry->SetSQL(tTabInfo.m_sSQL.c_str());
            m_pCurTabQry->Open();
            iFieldCnt = m_pCurTabQry->FieldCount();
            while(m_pCurTabQry->Next())
            {
                SetData(iFieldCnt, sizeof(m_sDataBuff));
                iRet = m_pCurTabWriter->Write(m_sDataBuff);
                if(iRet < 0)
                {
                    TADD_ERROR(-1,"bak data to file failed.");
                    return iRet;
                }

                iBakCount++;
                if(iBakCount%10000 == 0)
                {
                    TADD_NORMAL("Table[%s] bak data count[%lld]", tTabInfo.m_sTableName.c_str(), iBakCount);
                }
            }

            // 将缓存中剩余的数据写入文件
            iRet = m_pCurTabWriter->Write();
            if(iRet < 0)
            {
                TADD_ERROR(-1,"bak residue data to file failed.");
                return iRet;
            }

            TADD_NORMAL("Bak Table[%s] finish, [%lld] records baked.", tTabInfo.m_sTableName.c_str(), iBakCount);        
            
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            return -1;
        }
        catch(...)
        {
            TADD_ERROR(-1,"Unkown Exception.");
            return -1;
        }

        return 0;
    }

    void TOverDueDataCtrl::SetData(int iFieldCount, int iBuffLen)
    {
        int iPos = 6;
        
        for(int i = 0; i<iFieldCount;i++)
        {
            m_sDataBuff[iPos] = ',';
            ++iPos;
            m_sDataBuff[iPos] = '@';
            ++iPos;
            char* sFiledName = m_pCurTabQry->Field(i).GetName();
            snprintf(m_sDataBuff+iPos,iBuffLen-iPos,"%s",sFiledName);
            iPos+=strlen(sFiledName);
            m_sDataBuff[iPos] = '=';
            ++iPos;
            if(m_pCurTabQry->Field(i).isNULL())
            {
                snprintf(m_sDataBuff+iPos,iBuffLen-iPos,"%s","(nil)");
                iPos+=5;
            }
            else
            {
                char* sFieldValue=m_pCurTabQry->Field(i).AsString();
                snprintf(m_sDataBuff+iPos,iBuffLen-iPos,"%s",sFieldValue);
                iPos+=strlen(sFieldValue);		
            }		
        }

        snprintf(m_sDataBuff+iPos,iBuffLen-iPos,"&&");
        iPos+=2;
        m_sDataBuff[iPos]='\0';

        m_sDataBuff[0] = '^';
        m_sDataBuff[1] = '^';
        m_sDataBuff[2] = (iPos)/1000 + '0';
        m_sDataBuff[3] = ((iPos)%1000)/100 + '0';
        m_sDataBuff[4] = ((iPos)%100)/10 + '0';
        m_sDataBuff[5] = ((iPos)%10) + '0';
        
        return ;
    }

    int TOverDueDataCtrl::InitTabWriter(const TTableCleanInfo& tTabInfo)
    {
        if(NULL == m_pCurTabWriter)
        {
            m_pCurTabWriter = new(std::nothrow) TRecordWriter();
            if(NULL == m_pCurTabWriter)
            {
                TADD_ERROR(-1,"new TRecordWriter failed.");
                return -1;
            }
        }

        char sFileName[MAX_FILE_NAME] = {0};
        char sCurTime[MAX_TIME_LEN] ={0};

        TMdbDateTime::GetCurrentTimeStr(sCurTime);

        char sUpperTabName[MAX_NAME_LEN] ={0};
        char sUpperDsnName[MAX_NAME_LEN]={0};
        SAFESTRCPY(sUpperTabName, sizeof(sUpperTabName), tTabInfo.m_sTableName.c_str());
        SAFESTRCPY(sUpperDsnName, sizeof(sUpperDsnName), tTabInfo.m_sDsn.c_str());
		//mjx sql tool add start
		TMdbNtcStrFunc::ToUpper(sUpperTabName);
		TMdbNtcStrFunc::ToUpper(sUpperDsnName);
		//mjx sql tool add end
        // 文件名格式: DSN_TABLE_YYYYMMDDHHMMSS
        snprintf(sFileName, MAX_FILE_NAME, "BAK.%s.%s.%s.OK"
                    , sUpperDsnName, sUpperTabName, sCurTime);

        m_pCurTabWriter->Clear();

        if(m_pCurTabWriter->Init(m_sBakPath.c_str(), sFileName) < 0)
        {
            TADD_ERROR(-1,"Init table data file writer failed.");
            return -1;
        }

        return 0;
    }

    int TOverDueDataCtrl::RestoreAllTable()
    {
        int iRet = 0;
        m_tFileList.Clear();
        CHECK_RET(m_tFileList.Init(m_sBakPath.c_str()),"Init bak path file list handler failed.");
        CHECK_RET(m_tFileList.GetFileList("BAK.", ".OK"),"Read File List failed.");

        char sFileName[MAX_FILE_NAME] = {0};
        char sDsnName[MAX_NAME_LEN] = {0};
        char sTabName[MAX_NAME_LEN] = {0};

        memset(sFileName, 0, sizeof(sFileName));
        while(0 == m_tFileList.Next(sFileName) )
        {
            memset(sDsnName, 0, sizeof(sDsnName));
            memset(sTabName, 0, sizeof(sTabName));

            TADD_NORMAL("Process File:[%s]", sFileName);
            
            if(GetTableInfoFromFileName(sFileName, sDsnName, sTabName) < 0)
            {
                TADD_ERROR(-1,"File[%s] is not valid bak file .", sFileName);
                continue;
            }

            if(RestoreOneTable(sTabName, sDsnName, sFileName) < 0)
            {
                TADD_ERROR(-1,"Restore table[%s], Dsn[%s] ovedue data failed..", sTabName,sDsnName);
                continue;
            }     

            memset(sFileName, 0, sizeof(sFileName));
        }

        return iRet;
        
    }


    int TOverDueDataCtrl::CleanAllTable()
    {
        int iRet = 0;
        m_tFileList.Clear();
        CHECK_RET(m_tFileList.Init(m_sBakPath.c_str()),"Init bak path file list handler failed.");
        CHECK_RET(m_tFileList.GetFileList("BAK.", ".OK"),"Read File List failed.");

        char sFileName[MAX_FILE_NAME] = {0};
        char sDsnName[MAX_NAME_LEN] = {0};
        char sTabName[MAX_NAME_LEN] = {0};

        memset(sFileName, 0, sizeof(sFileName));
        while(0 == m_tFileList.Next(sFileName) )
        {
            memset(sDsnName, 0, sizeof(sDsnName));
            memset(sTabName, 0, sizeof(sTabName));

            TADD_NORMAL("Process File:[%s]", sFileName);
            
            if(GetTableInfoFromFileName(sFileName, sDsnName, sTabName) < 0)
            {
                TADD_ERROR(-1,"File[%s] is not valid bak file .", sFileName);
                continue;
            }

            if(CleanOneTable(sTabName, sDsnName, sFileName) < 0)
            {
                TADD_ERROR(-1,"Clean table[%s], Dsn[%s] ovedue data failed..", sTabName,sDsnName);
                continue;
            }     

            memset(sFileName, 0, sizeof(sFileName));
        }

        return iRet;
        
    }

    int TOverDueDataCtrl::RestoreOneTable(const char* psTableName, const char* psDsnName, const char* psFileName)
    {
        int iRet = 0;
        CHECK_OBJ(psFileName);

        if(NULL == m_pFilePrc)
        {
            m_pFilePrc = new(std::nothrow) TFileProcesser();
            CHECK_OBJ(m_pFilePrc);
        }

        m_pFilePrc->Clear();

        m_pCurTabQry = GetMdbQuery(psDsnName);

        CHECK_RET(m_pFilePrc->Init(psFileName, psDsnName, psTableName, m_pCurTabQry, OPER_RESTORE),"Restore File proc init failed.");
        CHECK_RET(m_pFilePrc->Excute(),"Restore Excute failed.");    
        
        return iRet;
    }

    int TOverDueDataCtrl::CleanOneTable(const char* psTableName, const char* psDsnName, const char* psFileName)
    {
        int iRet = 0;
        CHECK_OBJ(psFileName);

        if(NULL == m_pFilePrc)
        {
            m_pFilePrc = new(std::nothrow) TFileProcesser();
            CHECK_OBJ(m_pFilePrc);
        }

        m_pFilePrc->Clear();

        m_pCurTabQry = GetMdbQuery(psDsnName);

        CHECK_RET(m_pFilePrc->Init(psFileName, psDsnName, psTableName, m_pCurTabQry, OPER_CLEAN),"clean file proc init failed.");
        CHECK_RET(m_pFilePrc->Excute(), "clean excute failed.");    
        
        return iRet;
    }


    TFileProcesser::TFileProcesser()
    {
        m_pMdbQry = NULL;
        m_fp = NULL;
        m_pMdbQry = NULL;
        m_pRecParser = NULL;

        m_eOperType = OPER_UNKOWN;
        m_psFileBuff = NULL;

        m_sDsnName.clear();
        m_sTabName.clear();
        m_sFileName.clear();

        m_iBuffPos = 0;
        m_iBuffLen = 0;
        m_iBuffDataLen = 0;

        memset(m_sRecdBuff, 0, sizeof(m_sRecdBuff));

        m_iResidueLen = 0;
        memset(m_sResidueBuff, 0, sizeof(m_sResidueBuff));

        m_iStatCnt = 0;

        memset(m_sSQL, 0, sizeof(m_sSQL));

        m_vPkSet.clear();

        

    }

    TFileProcesser::~TFileProcesser()
    {
        SAFE_DELETE(m_psFileBuff);
        SAFE_CLOSE(m_fp);
        SAFE_DELETE(m_pRecParser);
    }


    int TFileProcesser::Excute()
    {
        int iRet = 0;
        if(NULL == m_psFileBuff)
        {
            m_iBuffLen = 32*1024*1024;
            m_psFileBuff = new(std::nothrow) char[m_iBuffLen];
            TADD_FLOW("new buff size=[%lld]M", m_iBuffLen/1024/1024);
            if(NULL == m_psFileBuff)
            {
                iRet = ERR_OS_NO_MEMROY;
                TADD_ERROR(-1,"new m_psFileBuff failed");
                TADD_FUNC("Finish");
                return iRet;
            }
        }
        
        // 获取文件大小
        unsigned long long  iFileLen =0;
        TMdbNtcFileOper::GetFileSize(m_sFileName.c_str(),iFileLen);
        /*if(iFileLen  > m_iBuffLen)
        {
            SAFE_DELETE(m_psFileBuff);
            m_iBuffLen = iFileLen ;
            m_psFileBuff = new char[m_iBuffLen];
            TADD_FLOW("renew buff size=[%lld]M", m_iBuffLen/1024/1024);
            if(NULL == m_psFileBuff)
            {
                iRet = ERR_OS_NO_MEMROY;
                TADD_ERROR(-1,"new m_psFileBuff failed");
                TADD_FUNC("Finish");
                return iRet;
            }
        }*/

        TADD_DETAIL("file-length=%d", iFileLen);
        if(0 == iFileLen)
        {
            TADD_NORMAL("File[%s] is empty file.",m_sFileName.c_str());
            return iRet;
        }
        
        /*
        // 拷贝不完整数据到buff
        if(m_iResidueLen > 0)
        {
            memcpy(m_psFileBuff, m_sResidueBuff, m_iResidueLen);
        }
        */

        // 读取文件内容到buff
        SAFE_CLOSE(m_fp);
        m_fp = fopen(m_sFileName.c_str(), "rt");
        if(m_fp == NULL)
        {
            TADD_ERROR(-1,"[%s] fopen() failed.", m_sFileName.c_str());
            return -1;   
        }

        MDB_INT64 iReadLen = 30*1024*1024;
        MDB_INT64 iFileRemainSize = iFileLen;
        memset(m_psFileBuff, 0, m_iBuffLen);

        char sOperType[32] = {0};
        if(m_eOperType == OPER_CLEAN)
        {
            SAFESTRCPY(sOperType, sizeof(sOperType), "Clean");
        }
        else
        {
            SAFESTRCPY(sOperType, sizeof(sOperType), "Restore");
        }

        TADD_NORMAL("%s Table[%s] begin", sOperType,m_sTabName.c_str());
        
        while(iFileRemainSize > 0)
        {
            iReadLen= (iReadLen > iFileRemainSize?iFileRemainSize:iReadLen);

            if(m_iResidueLen > 0)
            {
                memcpy(m_psFileBuff, m_sResidueBuff, m_iResidueLen);
            }

            if(fread((void*)(m_psFileBuff+m_iResidueLen) , iReadLen, 1,m_fp) != 1)
            {
                TADD_DETAIL("m_iResidueLen=%lld, iReadLen=%lld", m_iResidueLen , iReadLen);
                CHECK_RET(ERR_APP_INVALID_PARAM,"fread failed,errno=%d,errstr=%s",errno,strerror(errno));
            }

            m_psFileBuff[m_iResidueLen+ iReadLen] = 0;
            m_iBuffDataLen = m_iResidueLen + iReadLen;
            m_iBuffPos = 0;
            
            m_sResidueBuff[0] = 0;
            m_iResidueLen = 0;
            

            CHECK_RET(ProcBuff(),"process buff failed.");
            iFileRemainSize = iFileRemainSize - iReadLen;
            
        }
        /*
        if(fread(m_psFileBuff+m_iResidueLen , iFileLen,1, m_fp) != 1)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"fread failed,errno=%d,errstr=%s",errno,strerror(errno));
        }
        
        m_psFileBuff[m_iResidueLen+ iFileLen] = 0;
        m_iBuffDataLen = m_iResidueLen + iFileLen;
        m_iBuffPos = 0;
        
        m_sResidueBuff[0] = 0;
        m_iResidueLen = 0;
        */

        TADD_NORMAL("%s Table[%s] finish. total count=[%lld]", sOperType,m_sTabName.c_str(), m_iStatCnt);

        return 0;
    }

    int TFileProcesser::ProcBuff()
    {
        TADD_DETAIL("file buff length = [%d], buff data length = [%d].", m_iBuffLen, m_iBuffDataLen);
        int iRet = 0;
        int iRecdLen = 0;
        // one record one time
        while(true)
        {
            
            if(m_iBuffPos + 2 >= m_iBuffDataLen)
            {
                TADD_DETAIL("buff residue length smaller than min record length, save incomplete data");
                SaveIncompleteData();
                break;
            }

            // 检测记录开头(Head:^^, 2Byte)
            if('^' != m_psFileBuff[m_iBuffPos] || '^' != m_psFileBuff[m_iBuffPos + 1])
            {
                
                m_iBuffPos++;
                continue;
            }

            iRecdLen = 0;

            // 解析记录长度(length: 4Byte)
            iRecdLen += (m_psFileBuff[m_iBuffPos+2] - '0') * 1000;
            iRecdLen += (m_psFileBuff[m_iBuffPos+3] - '0') * 100;

            iRecdLen += (m_psFileBuff[m_iBuffPos+4] - '0') * 10;
            iRecdLen += (m_psFileBuff[m_iBuffPos+5] - '0') * 1;

            
            // 校验长度
            if(iRecdLen > m_iBuffDataLen || iRecdLen <= 0 )
            {
                TADD_ERROR(-1,"check record length([%d]) failed", iRecdLen);
                m_iBuffPos += 2; // 按Head长度偏移
                continue;
            }

            // m_psFileBuff尾部记录不完整，保存到m_sResidueBuff缓存
            if(iRecdLen > (m_iBuffDataLen - m_iBuffPos))
            {
                TADD_DETAIL("record not integrated, save incomplete data");
                SaveIncompleteData();
                break;
            }
            

            memset(m_sRecdBuff, 0, sizeof(m_sRecdBuff));
            m_pRecParser->Clear();
            
            memcpy(m_sRecdBuff,  (char*)&m_psFileBuff[m_iBuffPos], iRecdLen);

                   
            if(m_pRecParser->Parse(m_sRecdBuff) < 0)
            {
                TADD_ERROR(-1,"parse failed,Record:[%s] ",m_sRecdBuff);
                m_iBuffPos += 2; 
                continue;
            }

            m_pRecParser->Print();
            

            m_iBuffPos += iRecdLen; // 按记录解析出来的长度偏移

            if(m_eOperType == OPER_CLEAN)
            {
                iRet = ExcuteClean(m_pRecParser);
            }
            else
            {
                iRet = ExcuteRestore(m_pRecParser);
            }
            
            if(0 > iRet)
            {
                TADD_ERROR(-1,"deal one record failed");
                continue;
            }
        
        }
        TADD_FUNC("Finish");
        return iRet;
    }

    void TFileProcesser::SaveIncompleteData()
    {
        m_iResidueLen = m_iBuffDataLen - m_iBuffPos;
        if(m_iResidueLen <= 0)
        {
            m_iResidueLen = 0;
            m_sResidueBuff[0] = 0;
            return ;
        }
        memcpy(m_sResidueBuff, (char*)&m_psFileBuff[m_iBuffPos], m_iResidueLen);
        m_sResidueBuff[m_iResidueLen] = 0;
        return;
    }

    void TFileProcesser::Clear()
    {
        m_eOperType = OPER_UNKOWN;
        SAFE_CLOSE(m_fp);
        m_pMdbQry = NULL;
        
        if(NULL != m_pRecParser)
        {
            m_pRecParser->Clear();
        }
        

        m_sDsnName.clear();
        m_sTabName.clear();
        m_sFileName.clear();

        m_iBuffPos = 0;
        m_iBuffLen = 0;
        m_iBuffDataLen = 0;

        memset(m_sRecdBuff, 0, sizeof(m_sRecdBuff));

        m_iResidueLen = 0;
        memset(m_sResidueBuff, 0, sizeof(m_sResidueBuff));

        m_iStatCnt = 0;

        memset(m_sSQL, 0, sizeof(m_sSQL));

        m_vPkSet.clear();
        
    }


    int TFileProcesser::Init(const char* psFileName, const char* psDsnName,const char* psTabName,TMdbQuery* pMdbQry, TDataOperType tOperType)
    {
        int iRet = 0;
        
        CHECK_OBJ(psFileName);
         CHECK_OBJ(psDsnName);
        CHECK_OBJ(psTabName);
        CHECK_OBJ(pMdbQry);

        m_sDsnName = psDsnName;
        m_sFileName = psFileName;
        m_sTabName = psTabName;
        m_pMdbQry = pMdbQry;
        m_eOperType = tOperType;

        m_iStatCnt = 0;

        m_pRecParser = new(std::nothrow) TRecordParser();
        CHECK_OBJ(m_pRecParser);

        
        if(m_eOperType == OPER_CLEAN)
        {
            CHECK_RET(GetPk(),"Get PK failed.");
            
            iRet = GenCleanSQL();

            try
            {
                m_pMdbQry->Close();
                m_pMdbQry->SetSQL(m_sSQL);
            }
            catch(TMdbException& oe)
            {
                TADD_ERROR(-1,"Err-msg=[%s], Err-SQL=[%s]", oe.GetErrMsg(), oe.GetErrSql());
                return -1;
            }
            catch(...)
            {
                TADD_ERROR(-1,"unknown exception]");
                return -1;
            }
        }
        /*
        // 考虑到动态新增列的可能性，执行首条记录时根据记录中的列构造sql
        else
        {
            iRet = GenRestoreSQL();
        }
        */  

        return iRet;
    }

    int TFileProcesser::GetPk()
    {
        int iRet = 0;

        char sPK[32] ={0};

        TColmItem tColm;
        int iColmPos = -1;

        try
        {
            // 查dba_tables找主键
            char sQTabSql[MAX_SQL_LEN] = "select  primary_key from dba_tables where table_name = :table_name";
            m_pMdbQry->Close();
            m_pMdbQry->SetSQL(sQTabSql);
            m_pMdbQry->SetParameter("table_name", m_sTabName.c_str());
            m_pMdbQry->Open();
            while(m_pMdbQry->Next())
            {
                SAFESTRCPY(sPK, sizeof(sPK),m_pMdbQry->Field("primary_key").AsString());
                TADD_DETAIL("PK:[%s]", sPK);
            }

            TMdbNtcSplit  tPkSplit; // dba_tables.primary_key分割解析
            tPkSplit.SplitString(sPK,',');

            // 查dba_column找主键列的列名
            char sQColmSql[MAX_SQL_LEN] = "select column_name ,data_type,data_pos  from dba_column where table_name = :table_name order by data_pos";
            m_pMdbQry->Close();
            m_pMdbQry->SetSQL(sQColmSql);
            m_pMdbQry->SetParameter("table_name", m_sTabName.c_str());
            m_pMdbQry->Open();
            while(m_pMdbQry->Next())
            {
                tColm.Clear();

                SAFESTRCPY(tColm.m_sName, sizeof(tColm.m_sName), m_pMdbQry->Field("column_name").AsString());
                iColmPos  = m_pMdbQry->Field("data_pos").AsInteger();

                for(int i = 0;i <tPkSplit.GetFieldCount(); i++)
                {
                    if(iColmPos == atoi(tPkSplit[i]))
                    {
                        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbQry->Field("data_type").AsString(), "DATESTAMP"))
                        {
                            tColm.m_bIsTime = true;
                        }

						//mjx sql tool add start
						SAFESTRCPY(tColm.m_sName,sizeof(tColm.m_sName),m_pMdbQry->Field("column_name").AsString());
						//mjx sql tool add end
                        TADD_DETAIL("Get PK:[%s]", tColm.m_sName);
                        m_vPkSet.push_back(tColm);
                        break;
                    }
                }
            }     
            
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-SQL=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            return -1;
        }
        catch(...)
        {
            TADD_ERROR(-1,"unknown exception]");
            return -1;
        }

        
        return iRet;
    }


    bool TFileProcesser::IsPK(const TColmItem& tColm)
    {
        bool bRet = false;
        
        std::vector<TColmItem>::iterator itor = m_vPkSet.begin();
        for(; itor !=m_vPkSet.end(); ++itor )
        {
            TColmItem& tColmPK = *itor;
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(tColmPK.m_sName, tColm.m_sName))
            {
                bRet =true;
                break;
            }
        }

        return bRet;
    }


    int TFileProcesser::ExcuteClean( TRecordParser* pRecd)
    {
        int iRet = 0;

        try
        {
            std::vector<TColmItem>::iterator itor = pRecd->m_vColmSet.begin();
            for(; itor != pRecd->m_vColmSet.end(); ++itor)
            {
                TColmItem & tColm = *itor;
                if(true == IsPK(tColm))
                {
                    m_pMdbQry->SetParameter(tColm.m_sName, tColm.m_sValue);
                }
            }
            
            m_pMdbQry->Execute();
            m_pMdbQry->Commit();

            m_iStatCnt++;

            if(m_iStatCnt%10000 == 0)
            {
                TADD_NORMAL("Clean Table[%s] count=[%lld]", m_sTabName.c_str(), m_iStatCnt);
            }
            
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-SQL=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            iRet = -1;
        }
        catch(...)
        {
            TADD_ERROR(-1,"unknown exception]");
            iRet = -1;
        }

        if(iRet < 0)
        {
            TADD_ERROR(-1,"Clean Faild.Dsn[%s] Table[%s] Record:[%s]"
                ,m_sDsnName.c_str(), m_sTabName.c_str(), pRecd->GetRecordStr());
        }

        return iRet;
    }

    int TFileProcesser::ExcuteRestore( TRecordParser* pRecd)
    {
        int iRet = 0;

        try
        {   
            if(strlen(m_sSQL) == 0)
            {
                CHECK_RET(GetColmIsTime(pRecd),"Get  column data type failed.");
                
                GenRestoreSQL(pRecd);
                m_pMdbQry->Close();
                m_pMdbQry->SetSQL(m_sSQL);
            }
            
            std::vector<TColmItem>::iterator itor = pRecd->m_vColmSet.begin();
            for(; itor != pRecd->m_vColmSet.end(); ++itor)
            {
                TColmItem & tColm = *itor;
                if(tColm.m_bNull == true)
                {
                    m_pMdbQry->SetParameterNULL(tColm.m_sName);
                }
                else
                {
                    m_pMdbQry->SetParameter(tColm.m_sName, tColm.m_sValue);
                }
            }
            
            m_pMdbQry->Execute();
            m_pMdbQry->Commit();

            m_iStatCnt++;

            if(m_iStatCnt%10000 == 0)
            {
                TADD_NORMAL("Restore Table[%s] count=[%lld]", m_sTabName.c_str(),m_iStatCnt);
            }
            
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-SQL=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            iRet = -1;
        }
        catch(...)
        {
            TADD_ERROR(-1,"unknown exception]");
            iRet = -1;
        }

        if(iRet < 0)
        {
            TADD_ERROR(-1,"Restore Faild.Dsn[%s] Table[%s] Record:[%s]"
                ,m_sDsnName.c_str(), m_sTabName.c_str(), pRecd->GetRecordStr());
        }

        return iRet;
    }

    int TFileProcesser::GenCleanSQL()
    {
        int iRet = 0;
        char sWhere[1024] = {0};
        std::vector<TColmItem>::iterator itor = m_vPkSet.begin();
        for(; itor != m_vPkSet.end(); ++itor)
        {
           TColmItem& tColmItem = *itor;
           if(tColmItem.m_bIsTime == true)
           {
               snprintf(sWhere+strlen(sWhere),sizeof(sWhere)-strlen(sWhere)," %s=to_date(:%s, 'YYYYMMDDHH24MISS') and",tColmItem.m_sName,tColmItem.m_sName);
           }
           else
           {
               snprintf(sWhere+strlen(sWhere),sizeof(sWhere)-strlen(sWhere)," %s=:%s and",tColmItem.m_sName,tColmItem.m_sName);
           }
        }

        TADD_DETAIL("WHERE:[%s]", sWhere);

        sWhere[strlen(sWhere)-3] = '\0';//去除最后一个'and'

        char sSQL[1024] = {0};
        snprintf(sSQL, sizeof(sSQL), "delete from %s where %s", m_sTabName.c_str(), sWhere);
        TADD_FLOW("DEL SQL:[%s]", sSQL);

        SAFESTRCPY(m_sSQL, sizeof(m_sSQL), sSQL);

        return iRet;
    }
    int TFileProcesser::GenRestoreSQL( TRecordParser* pRecd)
    {
        int iRet = 0;
        
        char sSql[MAX_SQL_LEN] = {0}; // 存放临时构建的sql
        char sColm[1024] = {0}; // 存放临时组织的列名
        char sColmValue[1024] = {0}; // 存放临时组织的列值

        std::vector<TColmItem>::iterator itor = pRecd->m_vColmSet.begin();
        for(;itor != pRecd->m_vColmSet.end(); ++itor)
        {
            TColmItem& tColm = *itor;
            
            snprintf(sColm+strlen(sColm), sizeof(sColm)-strlen(sColm),"%s,", tColm.m_sName);
            
            if(tColm.m_bIsTime == true)
            {
                snprintf(sColmValue+strlen(sColmValue),sizeof(sColmValue)-strlen(sColmValue)," to_date(:%s, 'YYYYMMDDHH24MISS'),",tColm.m_sName);
            }
            else
            {
                snprintf(sColmValue+strlen(sColmValue),sizeof(sColmValue)-strlen(sColmValue)," :%s,",tColm.m_sName);
            }
        }

        //去除最后一个，
        sColm[strlen(sColm) -1] = '\0';
        sColmValue[strlen(sColmValue) -1]   = '\0';
                    
        snprintf(sSql,sizeof(sSql),"insert into %s (%s) values(%s)", m_sTabName.c_str(),sColm,  sColmValue);

        TADD_FLOW("RESTORE SQL:[%s]", sSql);

        SAFESTRCPY(m_sSQL, sizeof(m_sSQL), sSql);

        return iRet;
    }


    int TFileProcesser::GetColmIsTime(TRecordParser* pRecd)
    {
        int iRet = 0;

        try
        {
            char sColName[MAX_NAME_LEN] = {0};        
            char sDataType[MAX_NAME_LEN]={0};        
            char sQColmSql[MAX_SQL_LEN] = "select column_name ,data_type  from dba_column where table_name = :table_name order by data_pos";
            
            m_pMdbQry->Close();
            m_pMdbQry->SetSQL(sQColmSql);
            m_pMdbQry->SetParameter("table_name", m_sTabName.c_str());
            m_pMdbQry->Open();
            while(m_pMdbQry->Next())
            {
                SAFESTRCPY(sColName, sizeof(sColName), m_pMdbQry->Field("column_name").AsString());
                SAFESTRCPY(sDataType, sizeof(sDataType), m_pMdbQry->Field("data_type").AsString());

                if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sDataType, "DATESTAMP"))
                {
                    std::vector<TColmItem>::iterator itor = pRecd->m_vColmSet.begin();
                    for(; itor != pRecd->m_vColmSet.end(); ++itor)
                    {
                        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sColName, itor->m_sName))
                        {
                            itor->m_bIsTime = true;
                            break;
                        }
                    }
                }           
                        
            }     
            
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-SQL=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            return -1;
        }
        catch(...)
        {
            TADD_ERROR(-1,"unknown exception]");
            return -1;
        }
       
        return iRet;
    }

//}    

