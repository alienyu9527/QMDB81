////////////////////////////////////////////////
// Name: mdbFileParser.cpp
// Author: Li.ShuGang
// Date: 2009/03/27
// Description: 读取minidb生成的日志文件,并进行解析
////////////////////////////////////////////////
/*
* $History: mdbFileParser.cpp $
 * 
 * *****************  Version 1.0  ***************** 
*/


#include "Helper/TThreadLog.h"
#include "Dbflush/mdbFileParser.h"
#include <sys/types.h>
#include <sys/stat.h>


//namespace QuickMDB{

    //内存数据库的某一个列的某一个数据
    TMdbData::TMdbData()
    {
        Clear();
    }


    TMdbData::~TMdbData()
    {

    }


    void TMdbData::Clear()
    {
        memset(sName, 0, sizeof(sName));
        memset(sPName, 0, sizeof(sPName));
        iType = -1;
        memset(sValue, 0, sizeof(sValue));
        cOperType = '=';
        iLen = 0;
        isNull = 0;
    }
#if 0

    //一个文件记录解析出来的数据结果
    TMdbOneRecord::TMdbOneRecord()
    {
        memset(m_sSQL, 0, sizeof(m_sSQL));
        m_iDataCounts = 0;
        m_pszRecord = NULL;
        for(int i = 0; i < MAX_TABLE_COUNTS;i++)
        {
            m_pQuery[i] = NULL;
        }
        m_pShmDSN = NULL;
        m_bIsAttach = false;
        m_bIsBlobTable = false;
    }


    TMdbOneRecord::~TMdbOneRecord()
    {
        if(m_pszRecord != NULL)
        {
            delete m_pszRecord;
            m_pszRecord = NULL;    
        }

        for(int i = 0;i<MAX_TABLE_COUNTS;i++)
        {
           SAFE_DELETE(m_pQuery[i]);
        }

        SAFE_DELETE(m_pShmDSN);
    }
    int TMdbOneRecord::Init()
    {
        int iRet = 0;
        CHECK_RET(CreateDBLink(),"CreateDBlink Fail");
        CHECK_RET(InitAllQuery(),"InitAllQuery Fail");
        return iRet;
    }

    int TMdbOneRecord::CreateDBLink()
    {
        m_pShmDSN = new TMdbShmDSN();
        m_pShmDSN->TryAttach();
        int iRet = m_pShmDSN->Attach(m_pConfig->GetDSN()->sName, *m_pConfig);
        if(iRet < 0)
        {
            TADD_FUNC("This Process Begin Start Type With Not Attach   .");
            SAFE_DELETE(m_pShmDSN);
            m_bIsAttach = false;
            return 0;  //默认成功.不注册,单独启动
        }
        else
        {
            m_bIsAttach = true;
        }
        char sUID[MAX_NAME_LEN], sPWD[MAX_NAME_LEN];
        memset(sUID, 0, sizeof(sUID));
        memset(sPWD, 0, sizeof(sPWD));
        int iflag = -1;
        for (int i = 0; i < m_pConfig->GetUserCounts(); i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pConfig->GetUser(i)->sAccess,"Administrator") == 0)
            {
                iflag = 0;
                strncpy(sUID, m_pConfig->GetUser(i)->sUser, MAX_NAME_LEN-1);
                strncpy(sPWD, m_pConfig->GetUser(i)->sPwd, MAX_NAME_LEN-1);
            }
        }

        if (iflag < 0)
        {
        	TADD_ERROR(ERROR_UNKNOWN,"[%s : %d] : TMdbOneRecord::Init() : No administrator.", __FILE__, __LINE__);
        	return -1;
        }

        
        //创建数据库链接
        try
        {
    	    m_tMinidb.SetLogin(sUID, sPWD, m_pConfig->GetDSN()->sName);
    	    m_tMinidb.Connect();
    	    m_tMinidb.DisableCommit();
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());   
            return ERR_DB_NOT_CONNECTED;
        }
        catch(...)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"Unknown error!\n");       
            return ERR_DB_NOT_CONNECTED;
        }

        return 0;
    }

    int TMdbOneRecord::DelTableQuery(const int iTableId)
    {
        int iRet = 0;
        
        if(iTableId >= 0  && iTableId < MAX_TABLE_COUNTS)
        {
            SAFE_DELETE(m_pQuery[iTableId]);
            TADD_NORMAL("Delete table query,table-id=%d", iTableId);
        }
        
        return iRet;
    }
    int TMdbOneRecord::AddTableQueryDynamic(const int iTableId)
    {

        if(m_bIsAttach == false || NULL == m_pShmDSN)
        {
            TADD_NORMAL("no support add table query dynamically when not attach shm.");
            return -1;
        }
        
        if(m_pQuery[iTableId] != NULL)
        {
            TADD_FLOW("TableId=%d, Table's query already exist.no need to create.", iTableId);
            return 0;
        }

        try
        {

            char m_sTempSQL[MAX_SQL_LEN];
            memset(m_sTempSQL,0,sizeof(m_sTempSQL));

            TMdbTable* pTmp = m_pShmDSN->GetTableById(iTableId);
            if(pTmp == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Table-Id[%d] not Exsit !pls check .", iTableId);
                return -1;
            }


            SAFESTRCPY(m_sTempSQL,sizeof(m_sTempSQL),"select * from ");
            snprintf(m_sTempSQL+strlen(m_sTempSQL),sizeof(m_sTempSQL), " %s where ", pTmp->sTableName);
            for(int k=0; k<pTmp->m_tPriKey.iColumnCounts; ++k)
            {
                int iColumnNo = pTmp->m_tPriKey.iColumnNo[k];
                if(k == 0)
                {	
                    snprintf(m_sTempSQL+strlen(m_sTempSQL),sizeof(m_sTempSQL),"%s=:%s", pTmp->tColumn[iColumnNo].sName, pTmp->tColumn[iColumnNo].sName);
                }
                else
                {	
                    snprintf(m_sTempSQL+strlen(m_sTempSQL),sizeof(m_sTempSQL)," and %s=:%s", pTmp->tColumn[iColumnNo].sName, pTmp->tColumn[iColumnNo].sName);
                }
            }

            m_pQuery[iTableId]  = m_tMinidb.CreateDBQuery();
            if(m_pQuery[iTableId]  == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Can't CreateDBQuery(Query).");
                return -1;     
            }    	    	    

            m_pQuery[iTableId]->SetSQL(m_sTempSQL);

        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
            return ERROR_UNKNOWN;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");       
            return ERROR_UNKNOWN;
        }
        
        return 0;
    }


    int TMdbOneRecord::InitAllQuery()
    {
        if(m_bIsAttach == false)
        {
            return 0;
        }
        
        try
        {

            char m_sTempSQL[MAX_SQL_LEN];
            memset(m_sTempSQL,0,sizeof(m_sTempSQL));
            //初始化query
            for(int i = 0;i<MAX_TABLE_COUNTS;i++)
            {
                TMdbTable* pTmp = m_pConfig->GetTableByPos(i);
                if(pTmp == NULL){continue;}

                memset(m_sTempSQL,0,sizeof(m_sTempSQL));
                SAFESTRCPY(m_sTempSQL,sizeof(m_sTempSQL),"select * from ");
                snprintf(m_sTempSQL+strlen(m_sTempSQL),sizeof(m_sTempSQL), " %s where ", pTmp->sTableName);
                for(int k=0; k<pTmp->m_tPriKey.iColumnCounts; ++k)
                {
                    int iColumnNo = pTmp->m_tPriKey.iColumnNo[k];
                    if(k == 0)
                    {	
                        snprintf(m_sTempSQL+strlen(m_sTempSQL),sizeof(m_sTempSQL),"%s=:%s", pTmp->tColumn[iColumnNo].sName, pTmp->tColumn[iColumnNo].sName);
                    }
                    else
                    {	
                        snprintf(m_sTempSQL+strlen(m_sTempSQL),sizeof(m_sTempSQL)," and %s=:%s", pTmp->tColumn[iColumnNo].sName, pTmp->tColumn[iColumnNo].sName);
                    }
                }

                if(m_pQuery[i] == NULL)
                {
                    m_pQuery[i]  = m_tMinidb.CreateDBQuery();
                    if(m_pQuery[i]  == NULL)
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"Can't CreateDBQuery(Query).", __FILE__, __LINE__);
                        return -1;     
                    }    	    	    
                }

                m_pQuery[i]->SetSQL(m_sTempSQL);

            }
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", __FILE__, __LINE__, e.GetErrSql(), e.GetErrMsg());   
            return ERROR_UNKNOWN;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n", __FILE__, __LINE__);       
            return ERROR_UNKNOWN;
        }
        return 0;
    }
    int TMdbOneRecord::PushData(int iPos,ST_COLUMN_VALUE & stColumnValue,bool bIsExit)
    {
        TMdbColumn * pColumn = &(m_pTable->tColumn[stColumnValue.iColPos]);
        m_tData[iPos].iLen  = pColumn->iColumnLen;
        m_tData[iPos].iType = pColumn->iDataType;
        m_tData[iPos].isNull = stColumnValue.bNull?-1:0;
        SAFESTRCPY(m_tData[iPos].sName,sizeof(m_tData[iPos].sName),pColumn->sName);
        SAFESTRCPY(m_tData[iPos].sPName,sizeof(m_tData[iPos].sPName),pColumn->sName);
        if(pColumn->iDataType == DT_Blob)
        {
            m_bIsBlobTable = true;
        }
        if(bIsExit == true)
        {
            if(m_pQuery[m_iTableID]->Field(stColumnValue.iColPos).isNULL() == true)
            {
                m_tData[iPos].isNull = -1;
            }
            else
            {
                SAFESTRCPY(m_tData[iPos].sValue,sizeof(m_tData[iPos].sValue),\
                    m_pQuery[m_iTableID]->Field(stColumnValue.iColPos).AsString());
            }
        }
        else
        {
            SAFESTRCPY(m_tData[iPos].sValue,sizeof(m_tData[iPos].sValue),stColumnValue.pValue);
        }
        return 0;
    }

    int TMdbOneRecord::Parse(unsigned char* pszRecord, int iLen)
    {
        int iRet = 0;
        int iRecordLen;
        CHECK_RET(m_tFlushData.AttachConfig(m_pConfig,m_pShmDSN),"AttachConfig faild.");
        CHECK_RET(m_tFlushData.DeSerialize((char *)pszRecord,iRecordLen),"DeSerialize error,iRet=[%d]",iRet);
        m_iTableID = m_tFlushData.iTableId;
        m_iType    = m_tFlushData.iSqlType;
        if(NULL != m_pShmDSN)
        {
            m_pTable = m_pShmDSN->GetTableById(m_iTableID);
        }
        else
        {
           m_pTable = m_pConfig->GetTableByID(m_iTableID);
        }
        memset(m_sSQL, 0, sizeof(m_sSQL));
        if(m_tFlushData.bNeedRepToOra() == false)
        {
            TADD_FLOW("table[%s] do not need to rep to oracle",m_pTable->sTableName);
            return -1;
        }

        if(NULL == m_pQuery[m_iTableID] && m_bIsAttach == true)
        {
            TADD_NORMAL("Table-Id=[%d], Table-Name=[%s] need to create mdb query", m_iTableID, m_pTable->sTableName);
            CHECK_RET(AddTableQueryDynamic(m_iTableID),"Create mdb query for table[%s(TableId:%d)] failed.", m_pTable->sTableName, m_iTableID);
        }
        
        CHECK_RET(m_tFlushData.GenSQL(m_sSQL,true),"GenSQL error.");
        if(m_iType == TK_UPDATE && m_bIsAttach == true)
        {
            bool bFlag = false;
            std::vector<ST_COLUMN_VALUE>::iterator itor = m_tFlushData.vWhereList.begin();
            for(;itor != m_tFlushData.vWhereList.end();++itor)
            {
                 ST_COLUMN_VALUE & stColumnValue = (*itor);
                 TMdbColumn * pColumn = &(m_pTable->tColumn[stColumnValue.iColPos]);
                 m_pQuery[m_iTableID]->SetParameter(pColumn->sName, stColumnValue.pValue);
            }
            m_pQuery[m_iTableID]->Open();
            if(m_pQuery[m_iTableID]->Next())
            {
                 bFlag = true;
            }

            m_iDataCounts = 0;
            itor = m_tFlushData.vColList.begin();
            for(;itor != m_tFlushData.vColList.end();++itor)
            {
                ST_COLUMN_VALUE & stColumnValue = (*itor);
                if(m_pTable->tColumn[stColumnValue.iColPos].bRepToOra())
                {
                    PushData(m_iDataCounts,stColumnValue,bFlag);
                    m_iDataCounts++;
                }
            }

            m_iWhereCounts = 0;
            itor = m_tFlushData.vWhereList.begin();
            for(;itor != m_tFlushData.vWhereList.end();++itor)
            {
                 ST_COLUMN_VALUE & stColumnValue = (*itor);
                    if(m_pTable->tColumn[stColumnValue.iColPos].bRepToOra())
                    {
                        PushData(m_iWhereCounts + m_iDataCounts,stColumnValue);
                        m_iWhereCounts++;
                    }
            }
        }
        else
        {
            //过滤不需要向oracle同步的列
           m_iDataCounts = 0;
           std::vector<ST_COLUMN_VALUE>::iterator itor = m_tFlushData.vColList.begin();
           for(;itor != m_tFlushData.vColList.end();++itor)
           {
                ST_COLUMN_VALUE & stColumnValue = (*itor);
                if(m_pTable->tColumn[stColumnValue.iColPos].bRepToOra())
                {
                    PushData(m_iDataCounts,stColumnValue);
                    m_iDataCounts++;
                }
           }
            m_iWhereCounts = 0;
            itor = m_tFlushData.vWhereList.begin();
            for(;itor != m_tFlushData.vWhereList.end();++itor)
            {
                ST_COLUMN_VALUE & stColumnValue = (*itor);
                if(m_pTable->tColumn[stColumnValue.iColPos].bRepToOra())
                {
                    PushData(m_iWhereCounts + m_iDataCounts,stColumnValue);
                    m_iWhereCounts++;
                }
            }
        }
        return iRet;
    }


    //处理一个文件,拆分出一条条的记录，并进行错误记录和记录规整 
    TMdbFileParser::TMdbFileParser()
    {
        TADD_FUNC("TMdbFileParser::TMdbFileParser() : Start.");
        
        m_pMemBuffer = NULL;  //一个文件的缓冲
        m_iMemSize   = 0;     //缓冲大小
        m_fp         = NULL;  //文件句柄
        m_errfp     =  NULL; //错误文件的文件句柄
        m_iPos       = 0;     //当前读取的位置
        m_iFileSize  = 0;     //当前文件大小
        m_pConfig    = NULL;  //配置文件
        m_pOneRecord = NULL;
        m_iOldPos = 0;//上一次读取的位置
        m_iLen = 0;
        
        TADD_FUNC("TMdbFileParser::TMdbFileParser() : Finish.");
    }


    TMdbFileParser::~TMdbFileParser()
    {
        TADD_FUNC("TMdbFileParser::~TMdbFileParser() : Start.");
        
        delete m_pMemBuffer;
        delete m_pOneRecord;
        
        SAFE_CLOSE(m_fp);

        SAFE_CLOSE(m_errfp); 
        
        TADD_FUNC("TMdbFileParser::~TMdbFileParser() : Finish.");
    }


    //设置读取的目录
    int TMdbFileParser::Init(const char* pszFullFileName, const char* pszDSN)
    {
        TADD_FUNC("TMdbFileParser::Init(%s) : Start.", pszFullFileName);
        
        //strcpy(m_sFullFileName, pszFullFileName);
        SAFESTRCPY(m_sFullFileName,sizeof(m_sFullFileName),pszFullFileName);
        //strcpy(m_sDSN, pszDSN);
        int iRet = 0;
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        if(NULL == m_pMemBuffer)    
        {
            m_pMemBuffer = new(std::nothrow) unsigned char[128*1024*1024];    
            if(m_pMemBuffer == NULL)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"new char[128M] failed, out of memory.");
                return ERR_OS_NO_MEMROY;    
            }
        }

        if(m_pOneRecord == NULL)
        {
            m_pOneRecord = new TMdbOneRecord();   
            m_pOneRecord->m_pszRecord = new char[MAX_VALUE_LEN];
            m_pOneRecord->m_pConfig = m_pConfig;
            if(m_pOneRecord->Init() < 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"init m_pOneRecord Fail.");
                return -1;
            }
        }
        
        //获取文件大小
        long iFileSize = GetFileSize(pszFullFileName);
        if(iFileSize < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbFileParser::Init(%s) : GetFileSize() failed, errno=%d, errmsg=[%s].", pszFullFileName, errno, strerror(errno));
        }
        
        //如果内存空间不足，则申请更大的内存
        if(iFileSize > m_iMemSize)
        {
            if(m_pMemBuffer != NULL)
                delete m_pMemBuffer;
            
            m_pMemBuffer = new(std::nothrow) unsigned char[iFileSize+1];    
            if(m_pMemBuffer == NULL)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"TMdbFileParser::Init(%s) : new char[%ld] failed, out of memory.",pszFullFileName, iFileSize+1);
                return ERR_OS_NO_MEMROY;    
            }
            m_iMemSize = iFileSize+1;
        }

        memset(m_pMemBuffer,0,m_iMemSize);
        
        //关闭原来的文件
        SAFE_CLOSE(m_fp);  

        //关闭原来打开的文件
        SAFE_CLOSE(m_errfp);
        m_iOldPos = 0;
        m_iLen = 0;
        
        //打开文件
        m_fp = fopen(pszFullFileName, "rt");
        if(m_fp == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbFileParser::Init(%s) : fopen() failed.", pszFullFileName);
            return -1;   
        }
        
        //把文件内容都读入内存中
        //long iFRet = fread(m_pMemBuffer, iFileSize, 1, m_fp);
        fread(m_pMemBuffer, iFileSize, 1, m_fp);
        m_iPos      = 0;
        m_iFileSize = iFileSize;
        
        TADD_FUNC("TMdbFileParser::Init(%s) : Finish(file-size=%ld).", pszFullFileName, m_iFileSize);
        
        return iRet;
    }


    //处理下一个记录
    TMdbOneRecord* TMdbFileParser::Next()
    {
        TADD_FUNC("TMdbFileParser::Next() : Start.");
        //add time:20120425 在读取文件时，保存上一次的m_iPos
        int iPosOldTemp = m_iPos;
        while(m_iPos < m_iFileSize)
        {
            TADD_DETAIL("TMdbFileParser::Next() : m_pMemBuffer[%d]=%c.", m_iPos, m_pMemBuffer[m_iPos]);
            TADD_DETAIL("TMdbFileParser::Next() : m_pMemBuffer[%d]=%c.", m_iPos+1, m_pMemBuffer[m_iPos+1]);
            
            if(m_pMemBuffer[m_iPos] == '^' && m_pMemBuffer[m_iPos+1] == '^')
            {
                int iLen   = (m_pMemBuffer[m_iPos+2]-'0')*1000 + (m_pMemBuffer[m_iPos+3]-'0')*100 +
                             (m_pMemBuffer[m_iPos+4]-'0')*10   + (m_pMemBuffer[m_iPos+5]-'0');
                if(iLen < 10 || m_pMemBuffer[m_iPos+iLen-1] != '#' || m_pMemBuffer[m_iPos+iLen-2] != '#')
                {
                    char sTemp[128] = { 0 };
                    strncpy(sTemp, (char*)&m_pMemBuffer[m_iPos], 127);
                    TADD_ERROR(ERROR_UNKNOWN,"Invalid data=[%s].", sTemp);
                    ++m_iPos;
                    continue;
                }
                //TADD_NORMAL("TMdbFileParser::Next() : iLen=%d.", iLen);
                int iRet = m_pOneRecord->Parse(&m_pMemBuffer[m_iPos], iLen-2);
                if(iRet < 0)
                {
                    //++m_iPos;   
                    //如果解析出错，则记录
                    m_iOldPos = m_iPos;
                    m_iLen = iLen;
                    //RecordError();
                    //TADD_ERROR("TMdbFileParser::Next() : Msg=[%s].", &m_pMemBuffer[m_iPos]);
                    m_iPos += iLen;
                	
                	return Next(); 
                }

                m_iOldPos = m_iPos;
                m_iLen = iLen;
                m_iPos += iLen;
                //add time:20120425 对比pos，如果相同，print info
                if(iPosOldTemp == m_iPos)
                {
                    TADD_ERROR(ERROR_UNKNOWN," SAME m_iPos(%d).",iPosOldTemp);
                }
                return m_pOneRecord;
            }
            else
            {
                ++m_iPos;    
            }
        }
            
        TADD_FUNC("TMdbFileParser::Next() : Finish(NULL).");
        
        return NULL;
    }

    bool TMdbFileParser::RecordError(const char* pszLogPath,const char* pszErrorLogPath)
    {
        TADD_FUNC("TMdbFileParser::RecordError() : Start.");
        char sErrorFile[512];
        char sFileName[256];
        memset(sErrorFile, 0, sizeof(sErrorFile));
        memset(sFileName,0,sizeof(sFileName));
        char *pFileName = m_sFullFileName+strlen(pszLogPath);
        strcpy(sFileName,pFileName);
        sprintf(sErrorFile, "%s%s_ERROR", pszErrorLogPath,sFileName);

        if(m_errfp == NULL)
        {
            m_errfp = fopen(sErrorFile, "a+");
            if(m_errfp == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"TMdbFileParser::RecordError(%s) : fopen() failed.", sErrorFile);
                return -1;   
            }
        }
        if(m_iOldPos+m_iLen > m_iFileSize)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbFileParser:m_iOldPos=[%ld],m_iLen=[%d],m_iFileSize=[%ld].",  m_iOldPos,m_iLen,m_iFileSize);
            return -1;
        }
        int iLenW = fwrite(&m_pMemBuffer[m_iOldPos], m_iLen, 1, m_errfp);	
        if (iLenW < 1)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbFileParser::Next() : fwrite() failed(iLenW=%d, iLen=%d), errno=%d, errmsg=[%s].", 
            iLenW, m_iLen, errno, strerror(errno));	
            return -1;
        }

        fflush(m_errfp);
        TADD_FUNC("TMdbFileParser::RecordError() : Finished.");
        return true;
    }


    long TMdbFileParser::GetFileSize(const char* pszFullFileName)
    {
        struct stat f_stat;
        if(stat(pszFullFileName, &f_stat) == -1) 
        {
            return -1;
        }
        return (long)f_stat.st_size;
    }   
    /******************************************************************************
    * 函数名称	:  ClearSQLCacheByTableId
    * 函数描述	:  清理sql缓存
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbFileParser::ClearSQLCacheByTableId(const int iTableId)
    {
        if(NULL != m_pOneRecord)
        {
            m_pOneRecord->m_tFlushData.ClearSQLCacheByTableId(iTableId);
            m_pOneRecord->DelTableQuery(iTableId);
        }
        return 0;
    }
    #endif

    TMdbOraRepFileStat::TMdbOraRepFileStat()
    {
        Clear();
    }

    TMdbOraRepFileStat::~TMdbOraRepFileStat()
    {
        
    }

    void TMdbOraRepFileStat::Clear()
    {
        for(int i=0;i<MAX_TABLE_COUNTS;i++)
        {
            m_RepCount[i].m_sTableName.clear();
            m_RepCount[i].m_iInsertCounts = 0;
            m_RepCount[i].m_iDeleteCounts = 0;
            m_RepCount[i].m_iUpdateCounts = 0;
        }
    }

    /******************************************************************************
    * 函数名称	:  Stat
    * 函数描述	:  统计同步文件中的每张表的操作情况
    * 输入		:  pOneRecord 同步记录
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbOraRepFileStat::Stat(TMdbLCR& tLCR)
    {
        if(tLCR.m_sTableName.length() == 0){return;}
        int iFreePos = -1;
        int i = 0;
        for(; i<MAX_TABLE_COUNTS;i++)
        {
            if(iFreePos == -1 && m_RepCount[i].m_sTableName.length() == 0)
            {
                iFreePos = i;
                break;
            }
            if(tLCR.m_sTableName == m_RepCount[i].m_sTableName){break;}
        }

        if(i >= MAX_TABLE_COUNTS)
        {
            if(iFreePos != -1)
            {
                m_RepCount[i].m_sTableName = tLCR.m_sTableName;
            }
            else
            {
                TADD_ERROR(-1,"ARRIVE MAX COUNT[500].");
                return;
            }
        }

        switch(tLCR.m_iSqlType)
        {
            case OP_Insert:
                m_RepCount[i].m_iInsertCounts++;
                break;
            case OP_Delete:
                m_RepCount[i].m_iDeleteCounts++;
                break;
            case OP_Update:
                m_RepCount[i].m_iUpdateCounts++;
                break;
            default:
                break;
        }
        return;
    }

    /******************************************************************************
    * 函数名称	:  PrintStatInfo
    * 函数描述	:  单元每个同步文件中每张表的操作情况
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbOraRepFileStat::PrintStatInfo()
    {
        for(int i=0;i<MAX_TABLE_COUNTS;i++)
        {
            if(m_RepCount[i].m_sTableName.length() == 0){continue;}
            TADD_NORMAL("Statistics: TableName = [%s],Insert = [%d],Update = [%d],Delete = [%d].",\
                m_RepCount[i].m_sTableName.c_str(),m_RepCount[i].m_iInsertCounts,m_RepCount[i].m_iUpdateCounts,m_RepCount[i].m_iDeleteCounts);
        }
    }

//}
