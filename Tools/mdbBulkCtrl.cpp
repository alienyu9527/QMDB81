#include "Tools/mdbBulkCtrl.h"
#include "Helper/TThreadLog.h"
//#include "Helper/mdbStrFunc.h"
//#include "Helper/mdbDateTimeFunc.h"
//#include "Helper/mdbSplit.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbErr.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;
//namespace QuickMDB{

#define BULK_NULL "NULL"//导入时识别文件中的空格式，导出时空输出为
 
#define BULK_IN_SET_PARA  do{\
if(TMdbNtcStrFunc::StrNoCaseCmp(BULK_NULL,pBegin) == 0)\
{\
    m_tConnCtrl.m_pQuery->SetParameterNULL(iColumnIndex);\
}\
else\
{\
    m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin);\
}\
}while(0)

TMdbConnectCtrl::TMdbConnectCtrl()
{
    m_pDBLink = NULL;
    m_pQuery = NULL;
    m_pOraDBLink = NULL;
    m_pOraQuery = NULL;
    
}

TMdbConnectCtrl::~TMdbConnectCtrl()
{
    Destroy();
}

/******************************************************************************
* 函数名称  :  Create()
* 函数描述  :  创建QUERY
* 输入		:  psDsnName DSN名称
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbConnectCtrl::CreateMdbQuery(const char * psDsnName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(psDsnName);
    //m_sDsn = psDsnName;
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
            psDsnName, e.GetErrSql(), e.GetErrMsg());
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbConnectCtrl::CreateOraQuery(const char * psDsnName)
{
    CHECK_OBJ(psDsnName);
    TMdbConfig *pMdbConfig = TMdbConfigMgr::GetMdbConfig(psDsnName);
    CHECK_OBJ(pMdbConfig);
    try
    {
        //m_pOraDBLink = new(std::nothrow) TOraDBDatabase();
        m_pOraDBLink = TMDBDBFactory::CeatDB();
        CHECK_OBJ(m_pOraDBLink);
        m_pOraDBLink->SetLogin(pMdbConfig->GetDSN()->sOracleUID, pMdbConfig->GetDSN()->sOraclePWD, pMdbConfig->GetDSN()->sOracleID);
        if(m_pOraDBLink->Connect() == false)
        {
            TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"Connect Oracle Faild,user=[%s],pwd=[%s],dsn=[%s].",
                pMdbConfig->GetDSN()->sOracleUID,pMdbConfig->GetDSN()->sOraclePWD,pMdbConfig->GetDSN()->sOracleID);
            return ERR_APP_CONNCET_ORACLE_FAILED;
        }

        m_pOraQuery = m_pOraDBLink->CreateDBQuery();
        CHECK_OBJ(m_pOraQuery);
    }
    catch(TMDBDBExcpInterface& e)//TMDBDBExcpInterface
    {
        TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"ERROR_MSG=%s\n",e.GetErrMsg());
        return ERR_APP_CONNCET_ORACLE_FAILED;
    }
    catch(...)
    {
        TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"Unknown error!\n");
        return ERR_APP_CONNCET_ORACLE_FAILED;
    }
    return 0;

}

/******************************************************************************
* 函数名称  :  Destroy()
* 函数描述  :  销毁QUERY
* 输入		:  无
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbConnectCtrl::Destroy()
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
    
    if(m_pOraQuery != NULL)
    {
        m_pOraQuery->Close();
    }
    
    if(m_pOraDBLink != NULL)
    {
        m_pOraDBLink->Disconnect();
    }
    SAFE_DELETE(m_pOraDBLink);
    SAFE_DELETE(m_pOraQuery);
    
    return 0;
}
//#if 0

TWriterCtrl::TWriterCtrl()
{
    m_pFile = NULL;
    Clear();
}

TWriterCtrl::~TWriterCtrl()
{
    SAFE_CLOSE(m_pFile);
}

/******************************************************************************
* 函数名称  :  Init()
* 函数描述  :  初始化
* 输入		:  sFileName 文件名(支持含路径的文件名)
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TWriterCtrl::Init(const char* sFileName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(sFileName);
    char sTempFile[MAX_PATH_NAME_LEN] = {0};
    char sPath[MAX_PATH_NAME_LEN] = {0};
    //Clear();
    //检查用户提供的文件名是否含有路径
    if(TMdbNtcStrFunc::FindString(sFileName,"/") != -1)
    {
        SAFESTRCPY(sTempFile,sizeof(sTempFile),TMdbNtcFileOper::GetFileName(sFileName));
        strncpy(sPath,sFileName,strlen(sFileName)-strlen(sTempFile));
        bool bExist = TMdbNtcFileOper::IsExist(sPath);
        if(!bExist)
        {
            if(!TMdbNtcDirOper::MakeFullDir(sPath))
            {
                TADD_ERROR(ERR_OS_CREATE_DIR,"Can't create dir=[%s].",sPath);
                return ERR_OS_CREATE_DIR;
            }
        }
    }
    m_pFile = fopen(sFileName, "wt");
    if(NULL == m_pFile)
    {
       TADD_ERROR(ERR_OS_OPEN_FILE,"Open file [%s] fail.",sFileName);
       return ERR_OS_OPEN_FILE;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称  :  Write()
* 函数描述  :  使用缓存写文件
* 输入		:  sData 需要写的数据
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TWriterCtrl::Write(char* sData,int iLen)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //int iLen = strlen(sData);
    if(m_iBufPos + iLen < MAX_SEND_BUF)
    {
        memcpy(&m_sBuf[m_iBufPos], sData, iLen);
        m_iBufPos+=iLen;
        
    }
    else
    {
        if(fwrite(m_sBuf, m_iBufPos, 1, m_pFile) != 1)
        {
            TADD_ERROR(ERR_OS_WRITE_FILE,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));
            return ERR_OS_WRITE_FILE;
        }
        fflush(m_pFile);
        m_iFileSize+=m_iBufPos;
        memcpy(m_sBuf, sData, iLen);
        m_iBufPos = iLen;
        
    }
    
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称  :  Write()
* 函数描述  :  直接写文件
* 输入		:  无
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TWriterCtrl::Write()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(m_iBufPos <= 0){return iRet;}
    if(fwrite(m_sBuf, m_iBufPos,1,m_pFile) != 1)
    {
        TADD_ERROR(ERR_OS_WRITE_FILE,"fwrite() failed, errno=%d, errmsg=[%s].",errno, strerror(errno));
        return ERR_OS_WRITE_FILE;
    }
    fflush(m_pFile);
    m_iFileSize+=m_iBufPos;
    m_iBufPos = 0;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称  :  Clear()
* 函数描述  :  清空
* 输入		:  无
* 输出		:  无
* 返回值    :  无
* 作者		:  
*******************************************************************************/
void TWriterCtrl::Clear()
{
    SAFE_CLOSE(m_pFile);
    memset(m_sFileName, 0, MAX_PATH_NAME_LEN);
    memset(m_sBuf, 0, sizeof(m_sBuf));
    m_iBufPos = 0;
    m_iFileSize = 0;
}

TMdbBuckOutData::TMdbBuckOutData()
{
    m_iDataSrc = 0;//默认从mdb
    m_pMdbTable = NULL;
    memset(m_sSelectSQL,0,sizeof(m_sSelectSQL));

	m_bAllTable = false;
	
	memset(m_sTsFormat,0,sizeof(m_sTsFormat));
	memset(m_sFilterToken,0,sizeof(m_sFilterToken));
	memset(m_sFileName,0,sizeof(m_sFileName));

	memset(m_sStrQuote,0,sizeof(m_sStrQuote));
	
	m_pShmDSN = NULL;
	
	
}

TMdbBuckOutData::~TMdbBuckOutData()
{
    m_tConnCtrl.Destroy();
}

/******************************************************************************
* 函数名称  :  Init()
* 函数描述  :  初始化
* 输入		:  pShmDSN指向共享内存对象， iFileFormat文件格式(1or2)，pDumpFile 导出文件名
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		: 
*******************************************************************************/
int TMdbBuckOutData::Init(TMdbShmDSN * pShmDSN,int iExptType,const char*pDumpFile,
    const char *sTableName,const char* sTsFormat,const char* sFilterToken,const char* sStrQuote,const char* pSQLFile)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pShmDSN);
    CHECK_OBJ(pDumpFile);
    CHECK_OBJ(sTableName);
	CHECK_OBJ(sTsFormat);
	CHECK_OBJ(sStrQuote);
	//日期时间的格式
	SAFESTRCPY(m_sTsFormat,sizeof(m_sTsFormat),sTsFormat);

	//全表或者单表的判断，处理
	if(TMdbNtcStrFunc::StrNoCaseCmp(sTableName, "all") == 0)
	{
		m_bAllTable = true;
		//dump数据的文件名
		SAFESTRCPY(m_sFileName,sizeof(m_sFileName),pDumpFile);
		m_pShmDSN = pShmDSN;
		//return 0;
	}

	//字段之间的分隔符,默认,
	if(sFilterToken[0] == 0)
	{
		m_sFilterToken[0] = ',';
		m_sFilterToken[1] = '\0';
	}

	else
	{
		m_sFilterToken[0] = sFilterToken[0];
		m_sFilterToken[1] = '\0';
	}

	//字符串的输出表示, 单引号或者双引号
	if(sStrQuote[0] == 0)
	{
		m_sStrQuote[0] = '\'';
		m_sStrQuote[1] = '\0';
	}

	else if(sStrQuote[0] == '\'' || sStrQuote[0] == '"')
	{
		m_sStrQuote[0] = sStrQuote[0];
		m_sStrQuote[1] = '\0';
	}
	else
	{
		TADD_ERROR(ERR_APP_INVALID_PARAM,"string quote is not valid");
		return ERR_APP_INVALID_PARAM;
	}
	
	//所有的表的导出，下面的暂时不做
	if(m_bAllTable ==  true)
		return 0;

    //写文件句柄
    CHECK_RET(m_tWriter.Init(pDumpFile),"Failed to initialize.");
    //内存表
    m_pMdbTable = pShmDSN->GetTableByName(sTableName);
    if(NULL == m_pMdbTable)
    {
        TADD_ERROR(ERR_TAB_NO_TABLE,"Table[%s] does not exist in the shared memory.",sTableName);
        return ERR_TAB_NO_TABLE;
    }
    if(IsBlobTable(m_pMdbTable))
    {
        TADD_ERROR(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Table[%s] containing BLOB columns cannot export data.",sTableName);
        return ERR_TAB_COLUMN_DATA_TYPE_INVALID;
    }
	if(iExptType ==  0)
		m_iDataSrc = 0;	//从mdb export
	else
		m_iDataSrc = 1;  //从oracle export

		
    //查询句柄，获取sql
    if(pSQLFile && pSQLFile[0] != 0) 
    {
       
        CHECK_RET(GetOraSQL(pSQLFile),"Failed to get query SQL,Table-name = %s.",sTableName);
        //CHECK_RET(m_tConnCtrl.CreateOraQuery(pShmDSN->GetInfo()->sName),"Create OraQuery failed.");
    }
    else
    {
    	
        CHECK_RET(GetMdbSQL(m_pMdbTable),"Failed to get query SQL,Table-name = %s.",sTableName);
        //CHECK_RET(m_tConnCtrl.CreateMdbQuery(pShmDSN->GetInfo()->sName),"Create MdbQuery failed.");
    }
	if(m_iDataSrc == 1)
	{
		CHECK_RET(m_tConnCtrl.CreateOraQuery(pShmDSN->GetInfo()->sName),"Create OraQuery failed.");

	}
	else
	{
		CHECK_RET(m_tConnCtrl.CreateMdbQuery(pShmDSN->GetInfo()->sName),"Create MdbQuery failed.");

	}
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称  :  ExportData()
* 函数描述  :  导出的全部数据
* 输入		:  
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
bool TMdbBuckOutData::FilterTable(TMdbTable* pTable)
{
	
	if(pTable->bIsSysTab)
	{
		return false;
	}
	
		
	return true;
}

int TMdbBuckOutData::ExportDataForAllTable()
{
	int iRet = 0;
	TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
    for(;itor != m_pShmDSN->m_TableList.end();++itor)
     {
         TMdbTable * pTable = &(*itor);
         if(pTable->sTableName[0]== 0){continue;}
         // 过滤系统表和表空间
         if(FilterTable(pTable) == false)
         {
             continue;
         }
		 //包含blob的列
		 if(IsBlobTable(m_pMdbTable))
    	{
        	continue;
    	}
          m_pMdbTable = pTable;  

		  char sFileName[MAX_PATH_FILE] ={0};
		  snprintf(sFileName,sizeof(sFileName),"%s_%s",m_sFileName,m_pMdbTable->sTableName);
		  
		  m_tWriter.Init(sFileName);

		  CHECK_RET(GetMdbSQL(m_pMdbTable),"Failed to get query SQL,Table-name = %s.",m_pMdbTable->sTableName);
        
    
		if(m_iDataSrc == 1)
		{
			//norep的表去掉
			 if(pTable->iRepAttr == REP_NO_REP)
        	{
            	continue;
        	}
			CHECK_RET(m_tConnCtrl.CreateOraQuery(m_pShmDSN->GetInfo()->sName),"Create OraQuery failed.");
			CHECK_RET(WriteTextFileFromOra(m_pMdbTable),"Failed to export SQL file,TableName = [%s].",m_pMdbTable->sTableName);
		}
		else
		{
			CHECK_RET(m_tConnCtrl.CreateMdbQuery(m_pShmDSN->GetInfo()->sName),"Create MdbQuery failed.");
			CHECK_RET(WriteTextFileFromMdb(m_pMdbTable),"Failed to export SQL file,TableName = [%s].",m_pMdbTable->sTableName);
		}

		m_tWriter.Write();
		m_tWriter.Clear();
		m_tConnCtrl.Destroy();
          
     }
     return iRet;

}


int TMdbBuckOutData::ExportData()
{
    TADD_FUNC("Start.");
    int iRet = 0;
	if(m_bAllTable == true)
	{
		ExportDataForAllTable();

	}
	else
	{
		if(1 == m_iDataSrc)
	    {//从ora导出
	        CHECK_RET(WriteTextFileFromOra(m_pMdbTable),"Failed to export SQL file,TableName = [%s].",m_pMdbTable->sTableName);
	    }
	    else
	    {//从 mdb导出
	        CHECK_RET(WriteTextFileFromMdb(m_pMdbTable),"Failed to export SQL file,TableName = [%s].",m_pMdbTable->sTableName);
	    }

	}
    
    TADD_FUNC("Finish.");
    return iRet;

}


/******************************************************************************
* 函数名称  :  IsBlobTable()
* 函数描述  :  判断表是否含有BLOB列
* 输入		:  pMdbTable 表对象
* 输出		:  无
* 返回值    :  true:含有BLOB列;false:不含BLOB列
* 作者		:  
*******************************************************************************/
bool TMdbBuckOutData::IsBlobTable(TMdbTable *pMdbTable)
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

int TMdbBuckOutData::FormateDate(char* sMdbDate,char* sFormatDate )
{
	char year[5] = {0};
	char month[3] = {0};
	char day[3] = {0};
	char hour[3] = {0};
	char mins[3] = {0};
	char secs[3] = {0};
						
	year[0] = sMdbDate[0];
	year[1] = sMdbDate[1];
	year[2] = sMdbDate[2];
	year[3] = sMdbDate[3];
						
	month[0] = sMdbDate[4];
	month[1] = sMdbDate[5];

	day[0] = sMdbDate[6];
	day[1] = sMdbDate[7];

	hour[0] = sMdbDate[8];
	hour[1] = sMdbDate[9];

	mins[0] = sMdbDate[10];
	mins[1] = sMdbDate[11];

	secs[0] = sMdbDate[12];
	secs[1] = sMdbDate[13];

	if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat, "YYYYMMDDHHMMSS") == 0)
		snprintf(sFormatDate,MAX_NAME_LEN,"%s%s%s%s%s%s",year,month,day,hour,mins,secs);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat, "YYYY-MM-DD HH:MM:SS") == 0)
		snprintf(sFormatDate,MAX_NAME_LEN,"%s-%s-%s %s:%s:%s",year,month,day,hour,mins,secs);

	else
		snprintf(sFormatDate,MAX_NAME_LEN,"%s%s%s%s%s%s",year,month,day,hour,mins,secs);
	return 0;				
}
/******************************************************************************
* 函数名称  :  WriteTextFileFromMdb()
* 函数描述  :  写文件，逗号分隔字段
* 输入		:  无
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbBuckOutData::WriteTextFileFromMdb(TMdbTable *pMdbTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iCount = 0;
    size_t  iLen = 0;
    char sValues[MAX_VALUE_LEN] = {0};
    CHECK_OBJ(pMdbTable);
    try
    {
        m_tConnCtrl.m_pQuery->Close();
        m_tConnCtrl.m_pQuery->SetSQL(m_sSelectSQL);
        m_tConnCtrl.m_pQuery->Open();
        while(m_tConnCtrl.m_pQuery->Next())
        {
            memset(sValues,0,sizeof(sValues));
            //sValues[0] = 0;
            for(int i = 0;i<pMdbTable->iColumnCounts;i++)
            {
                iLen = strlen(sValues);
                if(m_tConnCtrl.m_pQuery->Field(i).isNULL())
                {
                    snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,"%s%s",BULK_NULL,m_sFilterToken);
                    continue;
                }
                if(pMdbTable->tColumn[i].iDataType == DT_Int)
                {
                    snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,\
                        "%lld%s",m_tConnCtrl.m_pQuery->Field(i).AsInteger(),m_sFilterToken);
                }
                else if(pMdbTable->tColumn[i].iDataType == DT_DateStamp)
                {
                    

					if(m_sTsFormat[0] == 0)
                    {
                    	snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,\
                        "%s%s",m_tConnCtrl.m_pQuery->Field(i).AsString(),m_sFilterToken);
                	}
					// 'YYYYMMDDHHMMSS',YYYY-MM-DD HH:MM:SS
					else
					{
						char sMdbDate[MAX_TIME_LEN] = {0};
						char sFormatDate[MAX_NAME_LEN] = {0};
						SAFESTRCPY(sMdbDate,sizeof(sMdbDate),m_tConnCtrl.m_pQuery->Field(i).AsString());
						FormateDate(sMdbDate,sFormatDate);
						snprintf(sValues+iLen,\
                        	sizeof(sValues)-iLen,\
                        		"%s%s",sFormatDate,m_sFilterToken);
					}
                }
                else
                {
                	if(m_sStrQuote[0] == '"')
                    snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,\
                        "\"%s\"%s",m_tConnCtrl.m_pQuery->Field(i).AsString(),m_sFilterToken);
					else if(m_sStrQuote[0] == '\'')
					snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,\
                        "'%s'%s",m_tConnCtrl.m_pQuery->Field(i).AsString(),m_sFilterToken);
                }
            }
            iCount++;
            
            iLen = strlen(sValues);
            sValues[iLen-1] = '\n';//替换掉最后一个逗号
            CHECK_RET(m_tWriter.Write(sValues,iLen),"Failed to write ");
            if(iCount%100000 == 0) TADD_NORMAL("Count=%d",iCount);
        }
        m_tConnCtrl.m_pQuery->Close();
        
        CHECK_RET(m_tWriter.Write(),"Failed to write ");
        TADD_NORMAL("Count=%d",iCount);
    }
    catch(TMdbException& oe)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
        return ERR_SQL_INVALID;
    }
    catch(...)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Unkown Exception.");
        return ERR_SQL_INVALID;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbBuckOutData::WriteTextFileFromOra(TMdbTable *pMdbTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iCount = 0;
    int iLen = 0;
    char sValues[MAX_VALUE_LEN] = {0};
    CHECK_OBJ(pMdbTable);
    try
    {
        m_tConnCtrl.m_pOraQuery->Close();
        m_tConnCtrl.m_pOraQuery->SetSQL(m_sSelectSQL);
        m_tConnCtrl.m_pOraQuery->Open();
        while(m_tConnCtrl.m_pOraQuery->Next())
        {
            memset(sValues,0,sizeof(sValues));
            //sValues[0] = 0;
            for(int i = 0;i<pMdbTable->iColumnCounts;i++)
            {
                iLen = strlen(sValues);
                if(m_tConnCtrl.m_pOraQuery->Field(i).isNULL())
                {//空与空串的处理
                	//mjx sql tool add start
                    //NULL的处理要统一
                    //mjx sql tool add end
                    if(pMdbTable->tColumn[i].iDataType == DT_Char 
                        || pMdbTable->tColumn[i].iDataType == DT_VarChar)
                    {
                        snprintf(sValues+iLen,\
                            sizeof(sValues)-iLen,"%s%s",BULK_NULL,m_sFilterToken);
                    }
                    else
                    {
                        snprintf(sValues+iLen,\
                            sizeof(sValues)-iLen,"%s%s",BULK_NULL,m_sFilterToken);
                    }
                
                    continue;
                }
                if(pMdbTable->tColumn[i].iDataType == DT_Int)
                {
                    snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,\
                        "%lld%s",m_tConnCtrl.m_pOraQuery->Field(i).AsInteger(),m_sFilterToken);
                }
                else if(pMdbTable->tColumn[i].iDataType == DT_DateStamp)
                {
                	if(m_sTsFormat[0] == 0)
                    {
                    	snprintf(sValues+iLen,\
                        	sizeof(sValues)-iLen,\
                        		"%s%s",m_tConnCtrl.m_pOraQuery->Field(i).AsString(),m_sFilterToken);
                	}
					// 'YYYYMMDDHHMMSS',YYYY-MM-DD HH:MM:SS
					else
					{
						char sMdbDate[MAX_TIME_LEN] = {0};
						char sFormatDate[MAX_NAME_LEN] = {0};
						SAFESTRCPY(sMdbDate,sizeof(sMdbDate),m_tConnCtrl.m_pOraQuery->Field(i).AsString());
						FormateDate(sMdbDate,sFormatDate);
						snprintf(sValues+iLen,\
                        	sizeof(sValues)-iLen,\
                        		"%s%s",sFormatDate,m_sFilterToken);
					}
                }
                else
                {//字符串以单引号包括
                	if(m_sStrQuote[0] == '"')
                    snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,\
                        "\"%s\"%s",m_tConnCtrl.m_pOraQuery->Field(i).AsString(),m_sFilterToken);
					else if(m_sStrQuote[0] == '\'')
					snprintf(sValues+iLen,\
                        sizeof(sValues)-iLen,\
                        "'%s'%s",m_tConnCtrl.m_pOraQuery->Field(i).AsString(),m_sFilterToken);
                }
                
            }
            iCount++;
            
            iLen = strlen(sValues);
            sValues[iLen-1] = '\n';//替换掉最后一个逗号
            CHECK_RET(m_tWriter.Write(sValues,iLen),"Failed to write ");
            if(iCount%100000 == 0) TADD_NORMAL("Count=%d",iCount);
        }
        m_tConnCtrl.m_pOraQuery->Close();
        
        CHECK_RET(m_tWriter.Write(),"Failed to write ");
        TADD_NORMAL("Count=%d",iCount);
    }
    catch(TMDBDBExcpInterface& oe)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
        return ERR_SQL_INVALID;
    }
    catch(...)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Unkown Exception.");
        return ERR_SQL_INVALID;
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* 函数名称  :  GetQuerySQL()
* 函数描述  :  获取指定表的查询SQL
* 输入		:  pMdbTable 表对象
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbBuckOutData::GetMdbSQL(TMdbTable *pMdbTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    memset(m_sSelectSQL,0,sizeof(m_sSelectSQL));
	//mjx sql tool add start
    //mjx 给出了文件，用文件的
    //mjx sql tool add end
    //首先拼写select部分
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
* 函数名称  :  GetOraSQL()
* 函数描述  :  获取指定表的查询SQL,从文件中获取sql
* 输入      : 
* 输出      :  无
* 返回值    :  0 - 成功!0 -失败
* 作者      :  
    *******************************************************************************/
int TMdbBuckOutData::GetOraSQL(const char* pOraSQLFile)
{
    if(!pOraSQLFile || 0 == pOraSQLFile[0])
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"invalid name for SQL file ,name=[%s]",(pOraSQLFile==NULL)?"NULL":"");
        return ERR_APP_INVALID_PARAM;
    }
    
    FILE *pFile = fopen(pOraSQLFile, "rt");
    if(NULL == pFile)
    {
       TADD_ERROR(ERR_OS_OPEN_FILE,"Open file [%s] fail.",pOraSQLFile);
       return ERR_OS_OPEN_FILE;
    }
    int n = fread(m_sSelectSQL, sizeof(m_sSelectSQL)-1, 1, pFile);
    if(ferror(pFile))
    {
        
        TADD_ERROR(ERR_OS_READ_FILE,"fread()file[%s][%d]failed, errno=%d, errmsg=[%s].", pOraSQLFile,n,
            errno, strerror(errno));
        SAFE_CLOSE(pFile);
        return ERR_OS_READ_FILE;
    }
    SAFE_CLOSE(pFile);
    TMdbNtcStrFunc::TrimRight(m_sSelectSQL,'\n');
    TMdbNtcStrFunc::TrimRight(m_sSelectSQL,' ');
    TMdbNtcStrFunc::TrimRight(m_sSelectSQL,';');
    //mjx sql tool add start
    //没有文件，自己拼接sql生成
    //mjx sql tool add end
    return 0;
    
}
//#endif
TMdbBuckInData::TMdbBuckInData()
{
    m_pFile = NULL;
    m_pMdbTable = NULL;
    memset(m_sInsertSQL,0,sizeof(m_sInsertSQL));
	
	memset(m_sTsFormat,0,sizeof(m_sTsFormat));
	memset(m_sFilterToken,0,sizeof(m_sFilterToken));
	memset(m_sStrQuote,0,sizeof(m_sStrQuote));
	
}

TMdbBuckInData::~TMdbBuckInData()
{
    SAFE_CLOSE(m_pFile);
}

int TMdbBuckInData::Init(TMdbShmDSN * pShmDSN,const char * pDumpFile,const char *sTableName,const char* sTsFormat,const char* sFilterToken, const char* sStrQuote)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pShmDSN);
    CHECK_OBJ(pDumpFile);
    CHECK_OBJ(sTableName);
    CHECK_OBJ(sTsFormat);

	SAFESTRCPY(m_sTsFormat,sizeof(m_sTsFormat),sTsFormat);

	if(sFilterToken[0] == 0)
	{
		m_sFilterToken[0] = ',';
		m_sFilterToken[1] = '\0';
	}

	else if(sFilterToken[0] == '\'' || sFilterToken[0] == '"')
	{
		m_sFilterToken[0] = sFilterToken[0];
		m_sFilterToken[1] = '\0';
	}
	else
	{
		TADD_ERROR(ERR_APP_INVALID_PARAM,"string quote is not valid");
		return ERR_APP_INVALID_PARAM;
	}

	//字符串的输出表示, 单引号或者双引号
	if(sStrQuote[0] == 0)
	{
		m_sStrQuote[0] = '\'';
		m_sStrQuote[1] = '\0';
	}

	else
	{
		m_sStrQuote[0] = sStrQuote[0];
		m_sStrQuote[1] = '\0';
	}
	
    m_pMdbTable = pShmDSN->GetTableByName(sTableName);
    if(NULL == m_pMdbTable)
    {
        TADD_ERROR(ERR_TAB_NO_TABLE,"Table[%s] does not exist in the shared memory.",sTableName);
        return ERR_TAB_NO_TABLE;
    }
    if(IsBlobTable(m_pMdbTable))
    {
        TADD_ERROR(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Table[%s] containing BLOB columns cannot import data.",sTableName);
        return ERR_TAB_COLUMN_DATA_TYPE_INVALID;
    }
    
    //判断文件是否存在
    if(!TMdbNtcFileOper::IsExist(pDumpFile))
    {
        TADD_ERROR(ERR_APP_FILE_IS_EXIST,"File[%s] does not exist.",pDumpFile);
        return ERR_APP_FILE_IS_EXIST;
    }
    //打开文件
    m_pFile = fopen(pDumpFile, "rt");
    if(m_pFile == NULL)
    {
        TADD_ERROR(ERR_OS_READ_FILE,"Failed to open file[%s].",pDumpFile);
        return ERR_OS_READ_FILE;   
    }
    //创建QUERY
    CHECK_RET(m_tConnCtrl.CreateMdbQuery(pShmDSN->GetInfo()->sName),"Create Query failed.");
    CHECK_RET(GetInsertSQL(),"GetInsertSQL faild");
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbBuckInData::ImportData()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_RET(ImportTextFile(),"Failed to import file.");
    TADD_FUNC("Finish.");
    return iRet;
}
int TMdbBuckInData::ImportTextFile()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iValidCount = 0;
    int iInValidCount = 0;
    
    char sBufMsg[MAX_VALUE_LEN] = {0};
    try
    {
        m_tConnCtrl.m_pQuery->Close();
        m_tConnCtrl.m_pQuery->SetSQL(m_sInsertSQL);
        while(fgets(sBufMsg, sizeof(sBufMsg), m_pFile) != NULL)
        {
            //int iSqlSeq = -1;
            //去除换行符\n \t 等
            TMdbNtcStrFunc::FormatChar(sBufMsg);
            //跳过空行以及注释航行
            if (sBufMsg[0] == '\n' || sBufMsg[0] == 0 || sBufMsg[0] == '#')
            {
                continue;
            }
            //解析
            //TADD_NORMAL("sBufMsg = %s",sBufMsg);
            iRet = ParseDataPlus(sBufMsg);
            if(iRet != 0)
            {
                TADD_ERROR(ERR_APP_DATA_INVALID,"table[%s],Invalid data: [%s]",m_pMdbTable->sTableName,sBufMsg);
                iInValidCount++;
                continue;
            }
            //if(iValidCount++%5000 == 0) 
            m_tConnCtrl.m_pQuery->Commit();
            iValidCount++;
            if(iValidCount%100000 == 0)
            {
                TADD_NORMAL("Count=%d.",iValidCount);
            }
        }
        //m_tConnCtrl.m_pQuery->Commit();  
        TADD_NORMAL("ValidCount=%d,InvalidCount=%d.",iValidCount,iInValidCount);
    
    }
    catch(TMdbException& oe)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
        return ERR_SQL_INVALID;
    }
    catch(...)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Unkown Exception.");
        return ERR_SQL_INVALID;
    }
    
    TADD_FUNC("Finish.");
    return iRet;
}

#if 0
int TMdbBuckInData::ImportTextFileRead()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iValidCount = 0;
    int iInValidCount = 0;
    
    char sBufMsg[MAX_VALUE_LEN] = {0};
    
    char sBufTrunc[MAX_VALUE_LEN] = {0};
    try
    {
    if(GetInsertSQL(m_sTableName) != 0)
    {
        TADD_ERROR("GetInsertSQL faild");
        return -1;
        
    }
    
    TMdbSplit mdbSplit;
    int  iTrunc = -1;
    m_tConnCtrl.m_pQuery->Close();
    m_tConnCtrl.m_pQuery->SetSQL(m_sInsertSQL);
    while(fread(sBufMsg, sizeof(sBufMsg), 1, m_pFile) == 1)
    {
        //int iSqlSeq = -1;
        //去除换行符\n \t 等
        TMdbStrFunc::Trim(sBufMsg,'\n');
        //跳过空行以及注释航行
        if (strlen(sBufMsg) == 0)
        {
            continue;
        }
        int iSplitCount = mdbSplit.Split(sBufMsg, '\n');
        for(int i=0; i<iSplitCount; i++)
        {
        //解析
        //TADD_NORMAL("sBufMsg = %s",sBufMsg);
            if(iTrunc == 0) 
            {
                iRet = ParseData(sBufTrunc);
                iTrunc = -1;
                sBufTrunc[0] = 0;
                i--;
            }
            else
                iRet = ParseData(mdbSplit.GetIndex(i));
            if(iRet == -1 && i == iSplitCount-1)
            {
                memccpy(sBufTrunc,mdbSplit.GetIndex(i),0,sizeof(sBufTrunc));
                iTrunc = 0;
            }
            else if(iRet == -1 && i == 0)
            {
                if(sBufTrunc[0] != 0)
                {
                    strncat(sBufTrunc,mdbSplit.GetIndex(i),sizeof(sBufTrunc));
                    //iTrunc = 1;
                }
            }
            else if(iRet != 0)
            {
                TADD_ERROR("Invalid data: %s.",sBufMsg);
                iInValidCount++;
                continue;
            }
            
            if(iValidCount++%5000 == 0) m_tConnCtrl.m_pQuery->Commit();            
            if(iValidCount%100000 == 0)
            {
                TADD_NORMAL("Count=%d.",iValidCount);
            }
        }
        if(iValidCount%5000 == 0) m_tConnCtrl.m_pQuery->Commit();            
        if(iValidCount%100000 == 0)
        {
            TADD_NORMAL("Count=%d.",iValidCount);
        }
    }
    m_tConnCtrl.m_pQuery->Commit();  
    TADD_NORMAL("ValidCount=%d,InvalidCount=%d.",iValidCount,iInValidCount);
    
    }
    catch(TMdbException& oe)
    {
        TADD_ERROR("Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
        return ERROR_SQL_INVALID;
    }
    catch(...)
    {
        TADD_ERROR("Unkown Exception.");
        return ERROR_SQL_INVALID;
    }
    
    TADD_FUNC("Finish.");
    return iRet;
}
#endif
//字符串中不带逗号
int TMdbBuckInData::ParseData(const char* pData)
{
    static TMdbNtcSplit mdbSplit;
    try
    {
    
        int iSplitCount = mdbSplit.SplitString(pData, ',');//字段以逗号分隔
        if(m_pMdbTable->iColumnCounts != iSplitCount) 
        {
            TADD_ERROR(ERR_APP_DATA_INVALID,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iSplitCount);
            return ERR_APP_DATA_INVALID;
        }
        for(int i=0; i<iSplitCount; ++i)
        {
            if(strcmp(BULK_NULL,mdbSplit[i]) != 0)
            {
                m_tConnCtrl.m_pQuery->SetParameter(i,mdbSplit[i]);
            }
            else
            {
                m_tConnCtrl.m_pQuery->SetParameterNULL(i);
            }
            
        }
        
        m_tConnCtrl.m_pQuery->Execute();
    }
    catch(TMdbException& oe)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
        return ERR_SQL_INVALID;
    }
    catch(...)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Unkown Exception.");
        return ERR_SQL_INVALID;
    }
    
    return 0;

}

int TMdbBuckInData::ParseDateStamp(char* pBegin,int iColumnIndex )
{
	//时间类型的格式解析
	int i = 0;
	char strDate[6][10] = {0};
	char * ptr = pBegin;
	char strSplit[10]={0};
	int iPos = 0;
	while(*ptr != 0)
	if(*ptr >= '0' && *ptr<='9')
	{
		strSplit[i]= *ptr;
		ptr++;
		i++;
	}
	else
	{
		ptr++;
		strSplit[i] = '\0';
		if(iPos<=5)
			SAFESTRCPY(strDate[iPos],10,strSplit);
		else
			break;
		i = 0;
							
		iPos++;
											
	}
	
	if(iPos <=5 )
	{
		if(i != 0)
		{
			strSplit[i] = '\0';
			SAFESTRCPY(strDate[iPos],10,strSplit);
		}
	}
		
	char strFormatDate[MAX_TIME_LEN] = {0};
	snprintf(strFormatDate,sizeof(strFormatDate),"%s%s%s%s%s%s",strDate[0],strDate[1],strDate[2],strDate[3],strDate[4],strDate[5]);
	m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,strFormatDate);

	return 0;

}
//字符串中带逗号的处理
int TMdbBuckInData::ParseDataPlus(const char* pData)
{
    try
    {
        static char sBuf[MAX_VALUE_LEN] = {0};//不操作源数据
        char *pBufEnd = (char*)memccpy(sBuf, pData, 0, MAX_VALUE_LEN);
        if(!pBufEnd) 
        {
            TADD_ERROR(ERR_TAB_RECORD_LENGTH_EXCEED_MAX,"data len >[%d]!!",MAX_VALUE_LEN);
            return ERR_TAB_RECORD_LENGTH_EXCEED_MAX;
        }
        char *pBegin = sBuf;
        char *pEnd = NULL;
        int iColumnIndex = 0;
        int iFoundIndex = -1;
        int iColumnEnd = m_pMdbTable->iColumnCounts-1;//
        bool bColumnEnd = false;
        //开始解析
        while(*pBegin || iColumnIndex < m_pMdbTable->iColumnCounts)
        {
            if(iColumnIndex > iColumnEnd)
            {
                TADD_ERROR(ERR_TAB_COLUMN_NUM_EXCEED_MAX,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iColumnIndex+1);
                return ERR_TAB_COLUMN_NUM_EXCEED_MAX;//字段太多
            }
            bColumnEnd = (iColumnIndex == iColumnEnd);
            //不合并分支--逻辑清晰一点
            if(m_pMdbTable->tColumn[iColumnIndex].iDataType == DT_Int)
            {//整数不会以引号开始
               // if((pEnd = strchr(pBegin,',')) != NULL)
               if((pEnd = strchr(pBegin,m_sFilterToken[0])) != NULL)
                {//123,
                    //if(iColumnIndex == iColumnEnd)
                    if(bColumnEnd)
                    {
                        TADD_ERROR(ERR_TAB_COLUMN_NUM_EXCEED_MAX,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iColumnIndex+2);
                        return ERR_TAB_COLUMN_NUM_EXCEED_MAX;//字段太多
                    }
                
                    *pEnd = 0;
                    BULK_IN_SET_PARA;
                    pBegin = pEnd + 1;
                    
                }
                else if(bColumnEnd)//if(iColumnIndex == iColumnEnd)
                {//最后一个字段
                    BULK_IN_SET_PARA;
                    *pBegin = 0;//解析结束打上标记，下同
                }
                else
                {//缺少字段
                    TADD_ERROR(ERR_TAB_MISSING_COLUMN,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iColumnIndex+1);
                    return ERR_TAB_MISSING_COLUMN;
                }
                
            }
            else
            {//字符串可以由引号，也可以不用

                if(*pBegin != m_sStrQuote[0])
                {//没有单引号
                   // if((pEnd = strchr(pBegin,',')) != NULL)
                   if((pEnd = strchr(pBegin,m_sFilterToken[0])) != NULL)
                    {
                        //if(iColumnIndex == iColumnEnd)
                        if(bColumnEnd)
                        {
                            TADD_ERROR(ERR_TAB_COLUMN_NUM_EXCEED_MAX,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iColumnIndex+2);
                            return ERR_TAB_COLUMN_NUM_EXCEED_MAX;//字段太多
                        }
                        *pEnd = 0;
						//以begin开头的字符串如果是时间类型

						if(m_pMdbTable->tColumn[iColumnIndex].iDataType == DT_DateStamp)
						{

							
							if(TMdbNtcStrFunc::StrNoCaseCmp(BULK_NULL,pBegin) == 0)
								m_tConnCtrl.m_pQuery->SetParameterNULL(iColumnIndex);
							else
							{

								
								if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYYMMDDHHMMSS") == 0)
								{
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin);
								}
								else if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYY-MM-DD HH:MM:SS") == 0)
								{
									ParseDateStamp(pBegin,iColumnIndex);
									
								}

								else if(m_sTsFormat[0] == 0)
								{								
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin);
								}
								else
								{
									TADD_ERROR(ERR_APP_INVALID_PARAM,"date format not support");
									return ERR_APP_INVALID_PARAM;
								}
									

							}

						}
						
						else
						{
							BULK_IN_SET_PARA;
						}
                        
                        pBegin = pEnd + 1;
                    }
                    else if(bColumnEnd)//if(iColumnIndex == iColumnEnd)
                    {
                        //BULK_IN_SET_PARA;
                        
						if(m_pMdbTable->tColumn[iColumnIndex].iDataType == DT_DateStamp)
						{

							
							if(TMdbNtcStrFunc::StrNoCaseCmp(BULK_NULL,pBegin) == 0)
								m_tConnCtrl.m_pQuery->SetParameterNULL(iColumnIndex);
							else
							{
								if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYYMMDDHHMMSS") == 0)
								{
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin);
								}
								else if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYY-MM-DD HH:MM:SS") == 0)
								{
									ParseDateStamp(pBegin,iColumnIndex);
									
								}

								else if(m_sTsFormat[0] == 0)
								{
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin);
								}
								else
								{
									TADD_ERROR(ERR_APP_INVALID_PARAM,"date format not support");
									return ERR_APP_INVALID_PARAM;
								}
									

							}

						}
						
						else
						{
							BULK_IN_SET_PARA;
						}
                        
                        *pBegin = 0;
                    }
                    else
                    {
                        TADD_ERROR(ERR_TAB_MISSING_COLUMN,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iColumnIndex+1);
                        return ERR_TAB_MISSING_COLUMN;
                    }
                    
                }
                else //if(*(pBegin) == '\'')
                {//有单引号
                    //if((pEnd = strstr(pBegin,"\',")) != NULL)
                    //iFoundIndex = TMdbNtcStrFunc::FindString(pBegin,"\',");//找结束位置
                    char strFilter[3] = {0};
					strFilter[0] = *pBegin;
					strFilter[1] = m_sFilterToken[0];
					strFilter[2] = '\0';
					

					iFoundIndex = TMdbNtcStrFunc::FindString(pBegin,strFilter);//找结束位置
					if(iFoundIndex > 0)
                    {//'abc',
                        if(bColumnEnd)//if(iColumnIndex == iColumnEnd)
                        {
                            TADD_ERROR(ERR_TAB_COLUMN_NUM_EXCEED_MAX,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iColumnIndex+2);
                            return ERR_TAB_COLUMN_NUM_EXCEED_MAX;//字段太多
                        }
                    
                        *(pBegin + iFoundIndex) = 0;
                        //*pEnd = 0;
                        if(m_pMdbTable->tColumn[iColumnIndex].iDataType == DT_DateStamp)
                        {
                        	if(TMdbNtcStrFunc::StrNoCaseCmp(BULK_NULL,pBegin+1) == 0)
								m_tConnCtrl.m_pQuery->SetParameterNULL(iColumnIndex);
							else
							{
								
								if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYYMMDDHHMMSS") == 0)
								{
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin+1);
								}
								else if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYY-MM-DD HH:MM:SS") == 0)
								{
									
									ParseDateStamp(pBegin+1,iColumnIndex);
									
								}

								else if(m_sTsFormat[0] == 0)
								{
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin+1);
								}
								else
								{
									TADD_ERROR(ERR_APP_INVALID_PARAM,"date format not support");
									return ERR_APP_INVALID_PARAM;
								}

							}

                        }
                        else
						{
							m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin+1);
                        }
                        pBegin += iFoundIndex + 2;
                    }
                    else if(bColumnEnd)//if(iColumnIndex == iColumnEnd)
                    {//'def'
                        *(pBufEnd-2) = 0;//去除最后一个单引号

						
                        if(m_pMdbTable->tColumn[iColumnIndex].iDataType == DT_DateStamp)
                        {
                        	if(TMdbNtcStrFunc::StrNoCaseCmp(BULK_NULL,pBegin+1) == 0)
								m_tConnCtrl.m_pQuery->SetParameterNULL(iColumnIndex);
							else
							{
								
								if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYYMMDDHHMMSS") == 0)
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin+1);
								else if(TMdbNtcStrFunc::StrNoCaseCmp(m_sTsFormat,"YYYY-MM-DD HH:MM:SS") == 0)
								{
									ParseDateStamp(pBegin+1,iColumnIndex);
									
								}

								else if(m_sTsFormat[0] == 0)
								{
									m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin+1);
								}
								else
								{
									TADD_ERROR(ERR_APP_INVALID_PARAM,"date format not support");
									return ERR_APP_INVALID_PARAM;
								}

							}

                        }
						else
						{
							m_tConnCtrl.m_pQuery->SetParameter(iColumnIndex,pBegin+1);
						}
                        
                        *pBegin = 0;
                    }
                    else
                    {
                        TADD_ERROR(ERR_TAB_MISSING_COLUMN,"Column Count[%d]!=[%d]",m_pMdbTable->iColumnCounts,iColumnIndex+1);
                        return ERR_TAB_MISSING_COLUMN;
                    }
                    
                }
            }
            iColumnIndex++;
            
        }
        //解析结束
        m_tConnCtrl.m_pQuery->Execute();
    }
    catch(TMdbException& oe)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
        return ERR_SQL_INVALID;
    }
    catch(...)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Unkown Exception.");
        return ERR_SQL_INVALID;
    }
    
    return 0;

}

int TMdbBuckInData::GetInsertSQL()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    memset(m_sInsertSQL, 0, sizeof(m_sInsertSQL));
    //首先拼写insert部分
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
    //拼写value部分
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

bool TMdbBuckInData::IsBlobTable(TMdbTable *pMdbTable)
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

TMdbBulkCtrl::TMdbBulkCtrl()
{
    m_pShmDSN = NULL;
    m_pDsn = NULL;
}

TMdbBulkCtrl::~TMdbBulkCtrl()
{
    
}

/******************************************************************************
* 函数名称  :  Init()
* 函数描述  :  初始化
* 输入		:  pDsn DSN名称
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbBulkCtrl::Init(const char * pDsn)
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
* 函数名称  :  ImportData()
* 函数描述  :  导入数据
* 输入		:  pDumpFile 数据文件
* 输出		:  无
* 返回值    :  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbBulkCtrl::ImportData(const char * pDumpFile,const char *sTableName,const char * sTsFormat,const char* sFilterToken,const char* sStrQuote)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pDumpFile);
    CHECK_OBJ(sTableName);
    CHECK_RET(m_tImportData.Init(m_pShmDSN,pDumpFile,sTableName,sTsFormat,sFilterToken,sStrQuote),"Failed to initialize.");
    CHECK_RET(m_tImportData.ImportData(),"Failed to import file[%s]",pDumpFile);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbBulkCtrl::ExportData(int iExptType,const char * pDumpFile,const char *sTableName,const char* pOraSQLFile,const char * sTsFormat,const char* sFilterToken,const char* sStrQuote)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pDumpFile);
    CHECK_OBJ(sTableName);
    CHECK_RET(m_tExportData.Init(m_pShmDSN,iExptType,pDumpFile,sTableName,sTsFormat,sFilterToken,sStrQuote,pOraSQLFile),"Failed to initialize.");
    CHECK_RET(m_tExportData.ExportData(),"Failed to import file[%s]",pDumpFile);
    TADD_FUNC("Finish.");
    return iRet;
}
//}

