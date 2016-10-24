#include "Helper/mdbRepRecd.h"
#include "Helper/TThreadLog.h"
//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    TRepHeadInfo::TRepHeadInfo()
    {
        Clear();
    }

    TRepHeadInfo::~TRepHeadInfo()
    {
    }

    void TRepHeadInfo::Clear()
    {
        m_iRoutID = DEFAULT_ROUTE_ID;
        m_iSqlType = -1;
        m_iLen = 0;
        m_iTimeStamp = 0;
        m_lLsn = 0;
        m_iVersion = 0;
        m_iSyncFlag = 0;
        m_iColmLen = 0;
        m_lRowId = 0;
        memset(m_sTableName, 0, sizeof(m_sTableName));
        //m_vColmName.clear();
    }

    void TRepHeadInfo::Print()
    {
        TADD_DETAIL("TEST:[Head]Tab=[%s], SqlType=[%d], Len=[%d], TimeStamp=[%lld], LSN=[%lld],Version=[%d], SyncFlag=[%d]."
            , m_sTableName, m_iSqlType, m_iLen, m_iTimeStamp, m_lLsn, m_iVersion, m_iSyncFlag);
    }

    int TRepHeadInfo::GetRoutingId()
    {
        return m_iRoutID;
    }

    TRepHeadInfo& TRepHeadInfo::operator=(const TRepHeadInfo& tHeadInfo)
    {
        this->m_iRoutID = tHeadInfo.m_iRoutID;
        this->m_iSqlType = tHeadInfo.m_iSqlType;
        this->m_iLen = tHeadInfo.m_iLen;
        this->m_iSyncFlag = tHeadInfo.m_iSyncFlag;
        this->m_iVersion = tHeadInfo.m_iVersion;
        this->m_iTimeStamp = tHeadInfo.m_iTimeStamp;
        this->m_lLsn = tHeadInfo.m_lLsn;
        this->m_iColmLen = tHeadInfo.m_iColmLen;
        this->m_lRowId = tHeadInfo.m_lRowId;
        SAFESTRCPY(this->m_sTableName, sizeof(this->m_sTableName), tHeadInfo.m_sTableName);
        return (*this);
    }

    TRepColm::TRepColm()
    {
        Clear();
    }

    TRepColm::~TRepColm()
    {
    }

    void TRepColm::Clear()
    {
        m_bNull = false;
        m_sColmName.clear();
        m_sColmValue.clear();
    }

    void TRepColm::Print()
    {
        TADD_DETAIL("TEST:COLUMN:[%s]=[%s], Null=[%s]"
            , m_sColmName.c_str(), m_sColmValue.c_str(), m_bNull?"TRUE":"FALSE");
    }

    TMdbRepRecd::TMdbRepRecd()
    {
        Clear();
    }

    TMdbRepRecd::~TMdbRepRecd()
    {
    }

    void TMdbRepRecd::Clear()
    {
        m_tHeadInfo.Clear();
        m_vColms.clear();
        m_vWColms.clear();
    }

    void TMdbRepRecd::Print()
    {
        TADD_DETAIL("------------------------------------------------------------");
        m_tHeadInfo.Print();
        
        TADD_DETAIL("Colums:");
        std::vector<TRepColm>::iterator iterColm = m_vColms.begin();
        for(; iterColm != m_vColms.end(); iterColm++)
        {
            iterColm->Print();
        }

        TADD_DETAIL("Where-Colums:");
        std::vector<TRepColm>::iterator iterWColm = m_vWColms.begin();
        for(; iterWColm != m_vWColms.end(); iterWColm++)
        {
            iterWColm->Print();
        }

        TADD_DETAIL("------------------------------------------------------------");
    }

    TMdbRep12Decode::TMdbRep12Decode()
    {
        
        m_pCurTab = NULL;
        memset(m_sTemp, 0, sizeof(m_sTemp));

        int iSize = sizeof(iFactor)/sizeof(int);
        for(int i = 0; i < iSize; i++)
        {
            iFactor[i] = 1;
            int j = 0;
            for(j = 0; j < i; j++)
            {
                iFactor[i] *= 10;
            }
        }

        for(int i = 0; i<MAX_COLUMN_COUNTS; i++)
        {
            m_NullColPos[i] = -1;
        }


    }

    TMdbRep12Decode::~TMdbRep12Decode()
    {
    }

    int TMdbRep12Decode::Analyse(const  char * pDataStr,TMdbConfig* pMdbConfig,TMdbRepRecd &tRecd)
    {
        TADD_FUNC("Start.Data=[%s].",pDataStr);
        int iRet = 0;
        int iPos = 0;
        CHECK_OBJ(pMdbConfig);

        // ^^
        if('^' != pDataStr[iPos] || '^' != pDataStr[iPos + 1])
        {
            TADD_ERROR(-1,"record(1.2) not begin with ^^");
            return 1;
        }
        iPos += 2;

        // length
        tRecd.m_tHeadInfo.m_iLen = GetNum(pDataStr + iPos,4);//获取数据长度
        if(tRecd.m_tHeadInfo.m_iLen < 10 || tRecd.m_tHeadInfo.m_iLen >= MAX_VALUE_LEN || pDataStr[tRecd.m_tHeadInfo.m_iLen -2] != '#' || pDataStr[tRecd.m_tHeadInfo.m_iLen -1] != '#' )
        {
            //数据未完全
            return 1;
        }
        iPos += 4;

        // tableid(table_name)
        int iTableId = GetNum(pDataStr + iPos,4);
        m_pCurTab = pMdbConfig->GetTableByTableId(iTableId);
        if(NULL == m_pCurTab)
        {
            TADD_ERROR(-1, "can't find table[%d]",iTableId);
            return -1;
        }
        SAFESTRCPY(tRecd.m_tHeadInfo.m_sTableName, sizeof(tRecd.m_tHeadInfo.m_sTableName), m_pCurTab->sTableName);
        iPos += 4;

        // sqltype
        tRecd.m_tHeadInfo.m_iSqlType = GetNum(pDataStr + iPos,4);
        if(TK_INSERT != tRecd.m_tHeadInfo.m_iSqlType && TK_DELETE != tRecd.m_tHeadInfo.m_iSqlType && TK_UPDATE != tRecd.m_tHeadInfo.m_iSqlType)
        {
            CHECK_RET(-1,"iSqlType[%d] error.",tRecd.m_tHeadInfo.m_iSqlType);
        }
        iPos += 4;

        // source_id
        tRecd.m_tHeadInfo.m_iSyncFlag= GetNum(pDataStr + iPos,4);
        iPos += 4;
        
        TADD_DETAIL("iLen=[%d],iTableId=[%d],iSqlType=[%d],iSyncFlag=[%d].",tRecd.m_tHeadInfo.m_iLen,iTableId,tRecd.m_tHeadInfo.m_iSqlType,tRecd.m_tHeadInfo.m_iSyncFlag);
        
        for(int i = 0; i<MAX_COLUMN_COUNTS; i++)
        {
            m_NullColPos[i] = -1;
        }
        GetNullPos(pDataStr + iPos);
        int DataLen = 0;

        // tRecd.m_vColms
        CHECK_RET(GetData(pDataStr + iPos,tRecd.m_vColms,DataLen),"GetData error.");
        iPos += DataLen;

        // tRecd.m_vWColms
        if('|' == pDataStr[iPos])
        {
            iPos ++;
            CHECK_RET(GetData(pDataStr + iPos,tRecd.m_vWColms,DataLen),"GetData error.");
        }

        tRecd.m_tHeadInfo.m_iRoutID = DEFAULT_ROUTE_ID;
        tRecd.m_tHeadInfo.m_iVersion = VERSION_DATA_12;
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRep12Decode::GetData(const char * pData,std::vector<TRepColm> & vData,int &iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iPos = 0;
        int iValuePairLen = 0;
        vData.clear();//清理
        bool bPush =false;
        while(pData[iPos] != '\0' && pData[iPos] != '|' && pData[iPos] !='#')//\0 和|为终止符
        {
            TRepColm stColumnValue;
            bPush = false;
            CHECK_RET(GetColumnValue(pData + iPos,stColumnValue,iValuePairLen, bPush),"GetColumnValue failed.");
            if(bPush)
            {
                vData.push_back(stColumnValue);
            }
            iPos += iValuePairLen;
        }
        iLen = iPos;
        TADD_FUNC("Finish.iLen=%d.",iLen);
        return iRet;
    }

    int TMdbRep12Decode::GetColumnValue(const char* pValuePair,TRepColm & stColumn,int &iValuePairLen, bool& bPush)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iPos = 0;
        int iColPos = GetNum(pValuePair + iPos,2);
        if(iColPos != 99)
        {
            stColumn.m_sColmName = m_pCurTab->tColumn[iColPos].sName;
            bPush = true;
        }
        iPos += 2;
        if(pValuePair[iPos] != '=')
        {
            CHECK_RET(-1,"iPos[%d] != '=' pValuePair[%s]",iPos,pValuePair);
        }
        ++iPos;

        int iBeginPos = iPos;
        while(',' != pValuePair[iPos] || '@' != pValuePair[iPos + 1])
        {
            //找到结尾为,@的地方
            ++iPos;
        }
        int iValueSize = iPos - 3;
        memset(m_sTemp, 0, sizeof(m_sTemp));
        strncpy(m_sTemp, pValuePair+iBeginPos, static_cast<size_t>(iValueSize) );
        stColumn.m_sColmValue = m_sTemp;
        
        iValuePairLen    = iPos + 2;//这个valuepair 的长度
        if(m_NullColPos[iColPos] != -1)
        {
            stColumn.m_bNull = true;
        }
        else
        {
            stColumn.m_bNull = false;
        }

        TADD_FUNC("Finish.ColPos[%d],Value[%s],size[%d].",
                  iColPos,stColumn.m_sColmName.c_str(),stColumn.m_sColmValue.c_str(),iValueSize);
        return iRet;
    }

    int TMdbRep12Decode::GetNum(const char* pNum,int iNumSize)
    {
        int iRetNum = 0;
        int i = 0;
        for(i = 0; i<iNumSize; i++)
        {
            iRetNum += iFactor[iNumSize - i -1]*(pNum[i] - '0');
        }
        return iRetNum;
    }

    void TMdbRep12Decode::GetNullPos(const char * pData)
    {
        int iPos = 0;
        int iStartPos = 0;
        int iLen = 0;
        char sNull[MAX_SQL_LEN];
        while(pData[iPos] != '\0' && pData[iPos] != '|' && pData[iPos] !='#')//\0 和|为终止符
        {

            if(pData[iPos] == ',' && pData[iPos+1] == '@' && pData[iPos+2] == '9' && pData[iPos+3] == '9' && pData[iPos+4] == '=')
            {
                iPos+=5;//包含一个=号，所以要多加一位
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

                memcpy(sNull,&pData[iPos],static_cast<size_t>(iLen));
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
                m_NullColPos[iCurPos] = iCurPos;
            }
        }
        return;
    }

    

    TMdbRepRecdDecode::TMdbRepRecdDecode()
    {
        m_bWhere = false;
        
        int iSize = sizeof(m_iFactor)/sizeof(int);
        for(int i = 0; i < iSize; i++)
        {
            m_iFactor[i] = 1;
            int j = 0;
            for(j = 0; j < i; j++)
            {
                m_iFactor[i] *= 10;
            }
        }
    }

    TMdbRepRecdDecode::~TMdbRepRecdDecode()
    {
    }

    int TMdbRepRecdDecode::GetNum(const  char* pNum,int iNumSize)
    {
        int iRetNum = 0;
        int i = 0;
        for(i = 0; i<iNumSize; i++)
        {
            iRetNum += m_iFactor[iNumSize - i -1]*(pNum[i] - '0');
        }
        return iRetNum;
    }

    int TMdbRepRecdDecode::GetRoutingID(const char* psData)
    {
        return GetNum(psData + 48, 4);
    }

    char TMdbRepRecdDecode::GetVersion(const char* psData)
    {
        char cVersion = *(psData+6);
        if(cVersion != VERSION_DATA_SYNC && cVersion != VERSION_DATA_CAPTURE && cVersion == '0')
        {
            cVersion = VERSION_DATA_12;
        }
        return cVersion;
    }
    
    int TMdbRepRecdDecode::GetHeadInfo(const char* psData, TRepHeadInfo& tHeadInfo, int& iHeadLen)
    {
        int iRet = 0;

        int iPos = 0;
        tHeadInfo.Clear();

        // ^^ [2 bytes]
        if('^' != psData[iPos] || '^' != psData[iPos + 1])
        {
            //头解析错误
            TADD_ERROR(-1,"Invalid Data!Data not begin with ^^");
            return -1;
        }
        iPos += 2;

        // record Length [4 bytes]
        int iLen = GetNum(psData + iPos,4);//获取数据长度
        if(iLen < 10 || iLen >= MAX_VALUE_LEN || psData[iLen -2] != '#' || psData[iLen -1] != '#' )
        {
            //数据未完全
            TADD_ERROR(-1,"Invalid Data!Data may not complete");
            return -1;
        }
        iPos += 4;
        tHeadInfo.m_iLen = iLen;

        // Version [1 bytes]
        tHeadInfo.m_iVersion = GetNum(psData + iPos, 1);
        iPos += 1;

        // SyncFlag [4 bytes]
        tHeadInfo.m_iSyncFlag =  GetNum(psData + iPos, 4);
        iPos += 4;

        // LSN [20 bytes]
        tHeadInfo.m_lLsn = GetNum(psData + iPos, 20);
        iPos += 20;

        // TimeStamp [10 bytes]
        tHeadInfo.m_iTimeStamp = GetNum(psData + iPos, 10);
        iPos += 10;

        // RowID [8 bytes]
        tHeadInfo.m_lRowId = GetNum(psData + iPos, 8);
        iPos += 8;

        // SQL Type [2 bytes]
        tHeadInfo.m_iSqlType = GetNum(psData + iPos, 2);
        iPos += 2;

        // Routing ID [4 bytes]
        tHeadInfo.m_iRoutID = GetNum(psData + iPos, 4);
        iPos += 4;

        // Column Name Length [4 bytes]
        tHeadInfo.m_iColmLen = GetNum(psData + iPos, 4);
        iPos += 4;

        // save headinfo length
        iHeadLen = iPos;

        // TableName
        int iBeginPos = iPos; 
        while(psData[iPos] != '\0' && psData[iPos] != '|'  && psData[iPos] != '#' )
        {
            if(psData[iPos] != ',')
            {
                iPos++;
                continue;
            }
            else
            {
                break;
            }            
        }
        //SAFESTRCPY(tHeadInfo.m_sTableName, iPos-iBeginPos, psData+iBeginPos);    
        strncpy(tHeadInfo.m_sTableName,psData+iBeginPos, static_cast<size_t>(iPos-iBeginPos));

        tHeadInfo.Print();

        return iRet;
        
    }

   

    int TMdbRepRecdDecode::DeSerialize(const  char * psData,TMdbRepRecd &tRecd)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        int iPos = 0;
        memset(m_sDataBuff, 0, sizeof(m_sDataBuff));
        tRecd.Clear();
        //m_vColm.clear();
        //m_vWColm.clear();
        CHECK_OBJ(psData);

        // 解析记录头信息
        CHECK_RET(GetHeadInfo(psData, tRecd.m_tHeadInfo, iPos)," parse head info failed.");

        // 解析表名列名串
        //SAFESTRCPY(m_sDataBuff, tRecd.m_tHeadInfo.m_iColmLen, psData+iPos);
        strncpy(m_sDataBuff, psData+iPos, static_cast<size_t>(tRecd.m_tHeadInfo.m_iColmLen));
        iPos += tRecd.m_tHeadInfo.m_iColmLen;
        int iColmNameCnt = 0;
        CHECK_RET(GetColmName(m_sDataBuff, tRecd,iColmNameCnt),"Decode column name failed.");

        // 解析列个数[4 bytes]
        int iColmCnt = GetNum(psData+iPos, 4);
        iPos += 4;

        if(iColmNameCnt != iColmCnt)
        {
            TADD_ERROR(-1,"Check column count failed.");
            return -1;
        }     

        int iColmLenPos = iPos;  // 列值长度起始偏移
        int iValuePos = iColmLenPos + iColmCnt * 4; // 列值起始偏移
        int iColumLen = 0; // 列值长度

        char sValue[MAX_VALUE_LEN] = {0};

        // 解析列
        std::vector<TRepColm>::iterator itor = tRecd.m_vColms.begin();
        for(; itor != tRecd.m_vColms.end(); ++itor)
        {
            iColumLen = GetNum(psData+iColmLenPos, 4);
            iColmLenPos += 4;
            
            if(iColumLen < 0)
            {
                itor->m_bNull = true;
                iValuePos += 0;
            }
            else
            {
                memset(sValue, 0, sizeof(sValue));
                //SAFESTRCPY(sValue, iColumLen, psData+iValuePos);
                strncpy(sValue,psData+iValuePos, static_cast<size_t>(iColumLen));
                itor->m_sColmValue = sValue;
                iValuePos += iColumLen;
            }
        }

        // 若带有where列，解析where列
        if(m_bWhere)
        {
            std::vector<TRepColm>::iterator itorW = tRecd.m_vWColms.begin();
            for(; itorW != tRecd.m_vWColms.end(); itorW++)
            {
                iColumLen = GetNum(psData+iColmLenPos, 4);
                iColmLenPos += 4;
                
                if(iColumLen < 0)
                {
                    itorW->m_bNull = true;
                    iValuePos += 0;
                }
                else
                {
                    memset(sValue, 0, sizeof(sValue));
                    //SAFESTRCPY(sValue, iColumLen, psData+iValuePos);
                    strncpy(sValue, psData+iValuePos, static_cast<size_t>(iColumLen));
                    itorW->m_sColmValue = sValue;
                    iValuePos += iColumLen;
                }
            }
        }

        tRecd.Print();

        return iRet;        
    }


    int TMdbRepRecdDecode::GetColmName(const char* psData,TMdbRepRecd &tRecd, int & iColmCnt)
    {
        int iRet = 0;

        bool bTabName = false;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(psData, '|');
        int iFieldCnt = static_cast<int>(tSplit.GetFieldCount());
        if(iFieldCnt == 1)
        {
            m_bWhere = false;
            tSplit.SplitString(psData, ',');
            for(unsigned int i = 0; i < tSplit.GetFieldCount(); i++)
            {
                if(strlen(tSplit[i]) == 0)
                {
                    continue;
                }
                
                if(bTabName ==false)
                {
                    bTabName = true;
                     strcpy(tRecd.m_tHeadInfo.m_sTableName, tSplit[i]);
                }
                else
                {
                    
                    TRepColm tColm;
                    tColm.m_sColmName = tSplit[i];
                    tRecd.m_vColms.push_back(tColm); 
                    iColmCnt++;
                }
            }
            
        }
        else if(iFieldCnt == 2)
        {
            m_bWhere = true; 
            char sTmp[MAX_VALUE_LEN] = {0};
            SAFESTRCPY(sTmp, sizeof(sTmp), tSplit[0]);
                        
            char sTmpW[MAX_VALUE_LEN]={0};
            SAFESTRCPY(sTmpW, sizeof(sTmpW), tSplit[1]);

            tSplit.SplitString(sTmp, ',');
            for(unsigned int i = 0; i < tSplit.GetFieldCount(); i ++)
            {
                if(strlen(tSplit[i]) == 0)
                {
                    continue;
                }
                
                if(bTabName ==false)
                {
                    bTabName = true;
                     strcpy(tRecd.m_tHeadInfo.m_sTableName, tSplit[i]);
                }
                else
                {
                    
                    TRepColm tColm;
                    tColm.m_sColmName = tSplit[i];
                    tRecd.m_vColms.push_back(tColm);  
                    iColmCnt++;
                }
            }

            tSplit.SplitString(sTmpW, ',');
            for(unsigned int i = 0; i < tSplit.GetFieldCount(); i ++)
            {
                if(strlen(tSplit[i]) == 0)
                {
                    continue;
                }
                    
                TRepColm tColm;
                tColm.m_sColmName = tSplit[i];
                tRecd.m_vWColms.push_back(tColm); 
                iColmCnt++;
            }
            
        }
        else
        {
            TADD_ERROR(-1,"more thran one '|', invalid column name string");
            iRet = -1;
        }
        return iRet;
    }


    TMdbRepRecdEnCode::TMdbRepRecdEnCode()
    {
        m_iColumCnt = 0;
        m_tRecd.Clear();
        ClearBuff();
    }

    TMdbRepRecdEnCode::~TMdbRepRecdEnCode()
    {
    }

    void TMdbRepRecdEnCode::AddHeadInfo(const TRepHeadInfo& tHeadInfo)
    {
        m_tRecd.m_tHeadInfo = tHeadInfo;
    }

    void TMdbRepRecdEnCode::AddColm(std::string sName, std::string sValue, bool bNull )
    {
        TRepColm tColm;
        tColm.m_sColmName = sName;
        tColm.m_sColmValue = sValue;
        tColm.m_bNull = bNull;
        m_tRecd.m_vColms.push_back(tColm);
    }

    void TMdbRepRecdEnCode::AddWhereColm(std::string sName, std::string sValue, bool bNull )
    {
        TRepColm tColm;
        tColm.m_sColmName = sName;
        tColm.m_sColmValue = sValue;
        tColm.m_bNull = bNull;
        m_tRecd.m_vWColms.push_back(tColm);
    }

    void TMdbRepRecdEnCode::SetColmCnt(const int iColmCnt)
    {
        m_iColumCnt = iColmCnt;
    }

    void TMdbRepRecdEnCode::ClearBuff()
    {
        memset(m_sNameBuff, 0, sizeof(m_sNameBuff));
        memset(m_sColmLenBuff, 0, sizeof(m_sColmLenBuff));
        memset(m_sColmValueBuff, 0, sizeof(m_sColmValueBuff));
    }


    int TMdbRepRecdEnCode::Serialize(char * pDataBuff,int iBuffLen, int &iRecdLen)
    {
        int iRet = 0;

        int iPos = 0;
        ClearBuff();

        // ^^
        pDataBuff[0] = '^';
        pDataBuff[1] = '^';
        iPos += 2;


        // record Length [4 bytes]
        // 长度等记录完整后再设置
        iPos += 4;

        // Version [1 bytes]
        pDataBuff[6] = '0'; // 目前version 默认为0
        iPos += 1;

        // TTL [1 bytes]
        pDataBuff[7] = '0';
        iPos += 1;
        
        // LSN [8 bytes]
        sprintf(pDataBuff +iPos , "%20lld", m_tRecd.m_tHeadInfo.m_lLsn);
        iPos += 8;
        
        // TimeStamp [8 bytes]
        sprintf(pDataBuff +iPos , "%08lld", m_tRecd.m_tHeadInfo.m_iTimeStamp);
        iPos += 8;
        
        // RowID [8 bytes]
        sprintf(pDataBuff +iPos , "%08lld", m_tRecd.m_tHeadInfo.m_lRowId);
        iPos += 8;
        
        // SQL Type [2 bytes]
        sprintf(pDataBuff +iPos , "%02d", m_tRecd.m_tHeadInfo.m_iSqlType);
        iPos += 2;
        
        // Routing ID [4 bytes]
        sprintf(pDataBuff +iPos , "%04d", m_tRecd.m_tHeadInfo.m_iRoutID);
        iPos += 2;

        int iNameLen  = 0;
        sprintf(m_sNameBuff, "%s",m_tRecd.m_tHeadInfo.m_sTableName);
        iNameLen += static_cast<int>(strlen(m_sNameBuff));

        int iColumLenPos = 0;
        int iColumValuePos = 0;
        int iValueLen = 0;

        std::vector<TRepColm>::iterator itor = m_tRecd.m_vColms.begin();
        for(; itor != m_tRecd.m_vColms.end(); ++itor )
        {
            sprintf(m_sNameBuff+iNameLen,",%s", itor->m_sColmName.c_str());
            iNameLen += (static_cast<int>(itor->m_sColmName.size()) + 1);

            if(itor->m_bNull)
            {
                 sprintf(m_sColmLenBuff + iColumLenPos, "%04d", NULL_VALUE_LEN);
                 iColumLenPos += 4;
            }
            else
            {
                iValueLen = static_cast<int>(itor->m_sColmValue.size());

                sprintf(m_sColmLenBuff + iColumLenPos, "%04d", iValueLen);
                iColumLenPos += 4;

                sprintf(m_sColmValueBuff + iColumValuePos, "%s", itor->m_sColmValue.c_str());
                iColumValuePos += iValueLen;
            }            
        }


        if(m_tRecd.m_vWColms.size() > 0)
        {
            m_sNameBuff[iColumLenPos] = '|';
            iColumLenPos +=1;

            std::vector<TRepColm>::iterator itorW = m_tRecd.m_vWColms.begin();
            for(; itorW != m_tRecd.m_vWColms.end(); itorW++ )
            {
                sprintf(m_sNameBuff+iNameLen,",%s", itorW->m_sColmName.c_str());
                iNameLen += (static_cast<int>(itorW->m_sColmName.size()) + 1);

                if(itorW->m_bNull)
                {
                     sprintf(m_sColmLenBuff + iColumLenPos, "%04d", NULL_VALUE_LEN);
                     iColumLenPos += 4;
                }
                else
                {
                    iValueLen = static_cast<int>(itorW->m_sColmValue.size());

                    sprintf(m_sColmLenBuff + iColumLenPos, "%04d", iValueLen);
                    iColumLenPos += 4;

                    sprintf(m_sColmValueBuff + iColumValuePos, "%s", itorW->m_sColmValue.c_str());
                    iColumValuePos += iValueLen;
                }            
            }
        }


        // Column Name Length [4 bytes]
        sprintf(pDataBuff +iPos , "%04d", iNameLen);
        iPos += 4;

        //SAFESTRCPY(pDataBuff +iPos, iNameLen, m_sNameBuff);
        strncpy(pDataBuff +iPos, m_sNameBuff, static_cast<size_t>(iNameLen));
        iPos += iNameLen;

        // Column count[4 bytes]
        sprintf(pDataBuff +iPos , "%04d", m_iColumCnt);
        iPos += 4;

        // 列值长度
        //SAFESTRCPY(pDataBuff +iPos, iColumLenPos, m_sColmLenBuff);
        strncpy(pDataBuff +iPos, m_sColmLenBuff, static_cast<size_t>(iColumLenPos));
        iPos += iColumLenPos;

        // 列值
        //SAFESTRCPY(pDataBuff +iPos, iColumValuePos, m_sColmValueBuff);
        strncpy(pDataBuff +iPos, m_sColmValueBuff, static_cast<size_t>(iColumValuePos));
        iPos += iColumValuePos;

        // ##
        pDataBuff[iPos] = '#';
        iPos++;
        pDataBuff[iPos] = '#';
        iPos++;

        iRecdLen = iPos;

        pDataBuff[2] = static_cast<char>((iRecdLen)/1000 + '0');
        pDataBuff[3] = static_cast<char>(((iRecdLen)%1000)/100 + '0');
        pDataBuff[4] = static_cast<char>(((iRecdLen)%100)/10 + '0');
        pDataBuff[5] = static_cast<char>(((iRecdLen)%10) + '0');

        TADD_FLOW("[%s]",pDataBuff );
        
        return iRet;
    }


//}
