#include "Tools/mdbExptDDLCtrl.h"
#include "Helper/TThreadLog.h"
//#include "Helper/mdbStrFunc.h"
//#include "Helper/mdbDateTimeFunc.h"
//#include "Helper/mdbSplit.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbErr.h"
#include "Helper/mdbEncrypt.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;
//namespace QuickMDB{



TMdbExptDDLCtrl::TMdbExptDDLCtrl()
{
 	sCfgFileName[0] = 0;
	sDsnName[0] = 0;
	sCfgDirName[0] = 0;
}

TMdbExptDDLCtrl::~TMdbExptDDLCtrl()
{

}

//dsn,config path初始化
int TMdbExptDDLCtrl::Init(const char* pstrDsn)
{
	int iRet = 0;
	SAFESTRCPY(sDsnName,sizeof(sDsnName),pstrDsn);
	TMdbNtcStrFunc::ToUpper(sDsnName);
	CHECK_RET(GetConfigHomePath(),"get config file for dsn failed");
	
	return iRet;

}

int TMdbExptDDLCtrl::GetConfigHomePath()
{
   	  int iRet = 0;


      if (NULL == getenv("QuickMDB_HOME"))
       {
            return -1;
        }
        char *pszHome = getenv("QuickMDB_HOME");


        char sConfigHome[MAX_PATH_NAME_LEN] = {0};

        SAFESTRCPY(sConfigHome,sizeof(sConfigHome),pszHome);
        if(sConfigHome[strlen(sConfigHome)-1] != '/')
        {
            sConfigHome[strlen(sConfigHome)] = '/';
        }
        char sTempzhy[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTempzhy,sizeof(sTempzhy),"%s.config/",sConfigHome);
        snprintf(sConfigHome,sizeof(sConfigHome), "%s",sTempzhy) ;


        SAFESTRCPY(sCfgDirName,sizeof(sCfgDirName),sConfigHome);
        
        memset(sTempzhy,0,sizeof(sTempzhy));
        snprintf(sTempzhy,sizeof(sTempzhy),"%s.%s/",sConfigHome,sDsnName);
        snprintf(sConfigHome,sizeof(sConfigHome), "%s",sTempzhy) ;
        if(TMdbNtcDirOper::IsExist(sConfigHome))
        {
            SAFESTRCPY(sCfgDirName,sizeof(sCfgDirName),sConfigHome);
        }
		else
			return -1;
		
        return iRet;
}

//获取表的属性的定义
int TMdbExptDDLCtrl::GetTablePropertyDefine(char* psTableDDL,MDBXMLElement* pMDB)
{
	for(MDBXMLElement* pEleId=pMDB->FirstChildElement(); pEleId;pEleId=pEleId->NextSiblingElement())
    {
           
			if(TMdbNtcStrFunc::StrNoCaseCmp(pEleId->Value(),"pkey") == 0 
				||TMdbNtcStrFunc::StrNoCaseCmp(pEleId->Value(),"column") == 0
				||TMdbNtcStrFunc::StrNoCaseCmp(pEleId->Value(),"index") == 0
				||TMdbNtcStrFunc::StrNoCaseCmp(pEleId->Value(),"name") == 0
				||TMdbNtcStrFunc::StrNoCaseCmp(pEleId->Value(),"table-level") == 0
				||TMdbNtcStrFunc::StrNoCaseCmp(pEleId->Value(),"is-PerfStat") == 0
			)
			 	continue;
			
			MDBXMLAttribute* pAttr = NULL;
			
			//pEleId->Value() "-"  to "_"
			char strAttrName[MAX_NAME_LEN] = {0};
			strncpy(strAttrName,pEleId->Value(),sizeof(strAttrName)-1);
			for(size_t i=0; i<strlen(strAttrName); i++)
				if (strAttrName[i] == '-')
					strAttrName[i] = '_';
				
			snprintf(psTableDDL+strlen(psTableDDL),MAX_SQL_LEN-strlen(psTableDDL),"%s=[\"",strAttrName);
            for(pAttr=pEleId->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                {
                    
                    snprintf(psTableDDL+strlen(psTableDDL),MAX_SQL_LEN-strlen(psTableDDL),"%s\"],",const_cast<char*>(pAttr->Value()));
                    break;
                }
            }
     }

	psTableDDL[strlen(psTableDDL)-1] = ';';
	return 0;
	

}

//索引定义
int TMdbExptDDLCtrl::GetIndexDefine(const char* psTableName,MDBXMLElement* pMDB,std::vector<string> & vecColName)
{

	char strInxDDL[MAX_SQL_LEN] = {0};
	
	 for(MDBXMLElement* pEle=pMDB->FirstChildElement("index"); pEle; pEle=pEle->NextSiblingElement("index"))
        {
            MDBXMLAttribute* pAttr = NULL;

		  	memset(strInxDDL,0,sizeof(strInxDDL));
			char strInxName[MAX_NAME_LEN] = {0};
			char strType[20] = {0};
			char strColName[MAX_NAME_LEN] = {0};
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(strInxName, pAttr->Value(), MAX_NAME_LEN-1);
                    TMdbNtcStrFunc::Trim(strInxName);
                   
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "algo-type") == 0)
                {
                    
                    int iType = atoi(pAttr->Value());
					if(iType == 0)
						strncpy(strType,"hash",sizeof(strType)-1);
					else if(iType == 1)
						strncpy(strType,"mhash",sizeof(strType)-1);
					else if(iType == 2)
						strncpy(strType,"btree",sizeof(strType)-1);
                    
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "column-pos") == 0)
                {
                    TMdbNtcSplit tSplit;
                    tSplit.SplitString(pAttr->Value(),',');
                    //tSplit.SetString(pAttr->Value());
                    for(unsigned int i=0; i<tSplit.GetFieldCount(); ++i)
                    {
                        int iPos = atoi(tSplit[i]);
						snprintf(strColName+strlen(strColName),sizeof(strColName)-strlen(strColName),"%s,",vecColName[iPos].c_str());
                    }
					strColName[strlen(strColName)-1] = '\0';
                }
                
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "max-layer") == 0)
                {
                   // pTable->tIndex[iIndexCounts].iMaxLayer= atoi(pAttr->Value());
                }
				else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "priority") == 0)
				{
				}
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].for index", pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }

			snprintf(strInxDDL,sizeof(strInxDDL)," create index %s on %s(%s) %s;",strInxName,psTableName,strColName,strType);
			printf("%s\n",strInxDDL);
	 }

	 

	 return 0;


}

//拼出列定义，主键定义
int TMdbExptDDLCtrl::GetColumnDefine(char* psTableDDL,MDBXMLElement* pMDB,std::vector<string> & vecColName)
{

	int iColumnCnt = 0;
	char strPk[MAX_NAME_LEN] = {0};

	//primary key
	//记录 主键的序号
	int iColPos[20] = {0};
	int iPkCnt = 0;
	 for(MDBXMLElement* pEle=pMDB->FirstChildElement("pkey"); pEle; pEle=pEle->NextSiblingElement("pkey"))
        {
            MDBXMLAttribute* pAttr = NULL;
          
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "column-pos") == 0)
                {
                     if(false == TMdbNtcStrFunc::IsDigital(pAttr->Value()))
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"Table PK config:column-pos =  [%s] is error,should be single number.",pAttr->Value());
						return ERR_APP_INVALID_PARAM;
                    }

					iColPos[iPkCnt] = atoi(pAttr->Value());
                    iPkCnt++;
                   
                }
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].",pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }
	 	}

	
	
	for(MDBXMLElement* pEle=pMDB->FirstChildElement("column"); pEle; pEle=pEle->NextSiblingElement("column"))
    {
		MDBXMLAttribute* pAttr = NULL;
		char strColName[MAX_NAME_LEN] = {0};
		char strType[20] = {0};
		char  strTypeLen[20]= {0};
		
		char iDefaultValue[256] = {0};
		bool bIsDefault = false;
		bool bNullable = false;
		size_t iLen = 0;
        for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
        {
            	
				
                SET_PARAM_BOOL_VALUE(bIsDefault,\
                    pAttr->Name(),"Is-Default",pAttr->Value());
                SET_PARAM_BOOL_VALUE(bNullable,\
                    pAttr->Name(),"Null-able",pAttr->Value());

				
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(strColName, pAttr->Value(), MAX_NAME_LEN-1);
					
                    TMdbNtcStrFunc::Trim(strColName);
					vecColName.push_back(strColName);
                }
                
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "data-type") == 0)
                {
                   
                    snprintf(strType,sizeof(strType),"%s",pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "data-len") == 0)
                {
                   	snprintf(strTypeLen,sizeof(strTypeLen),"%s",pAttr->Value());
                }
				
				
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"Default-Value") == 0)
                {
                    strncpy(iDefaultValue, pAttr->Value(), sizeof(iDefaultValue)-1);
                }
				/*
				else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"rep-type") == 0)
				{
				}
				else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"column-pos") == 0)
				{
				}
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].",pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
				*/
				
            }

		//主键拼起来
		for(int i=0; i<iPkCnt; i++)
			if(iColumnCnt == iColPos[i])
				snprintf(strPk+strlen(strPk),sizeof(strPk)-strlen(strPk),"%s,",strColName);
			
		snprintf(psTableDDL+strlen(psTableDDL),MAX_SQL_LEN-strlen(psTableDDL),"%s %s(%s)", strColName,strType,strTypeLen);

		if(bIsDefault)
			snprintf(psTableDDL+strlen(psTableDDL),MAX_SQL_LEN-strlen(psTableDDL)," default '%s' ",iDefaultValue);
		if(bNullable)
			snprintf(psTableDDL+strlen(psTableDDL),MAX_SQL_LEN-strlen(psTableDDL)," null");
		else
			snprintf(psTableDDL+strlen(psTableDDL),MAX_SQL_LEN-strlen(psTableDDL)," not null");

		iLen = strlen(psTableDDL);
		psTableDDL[iLen] = ',';
		psTableDDL[iLen+1] = '\0';
        iColumnCnt++;
     }

	

	//primary key strcat
	if(strPk[0] != 0)
	{
		strPk[strlen(strPk)-1] = '\0';
		snprintf(psTableDDL+strlen(psTableDDL),MAX_SQL_LEN-strlen(psTableDDL)," primary key(%s))",strPk);
	}
	
	size_t iLen = strlen(psTableDDL);
	psTableDDL[iLen -1] = ')';
	
		
	return 0;
}

//export sql:create table ......;create index.......
int TMdbExptDDLCtrl::ExptOneTableDDL(const char* psTableDir, const char* psTableName)
{
	
	 int iRet = 0;
	 char sCfgFile[MAX_FILE_NAME] = {0};
     snprintf(sCfgFile, sizeof(sCfgFile), "%s/.Tab_%s_%s.xml",psTableDir,sDsnName, psTableName);
	 MDBXMLDocument tDoc(sCfgFile);
     if (false == tDoc.LoadFile())
     {
        TADD_ERROR(-1,"Load table [%s] configuration failed. Please check if the configuration conforms to the XML syntax rules.", psTableName);
        return -1;
     }

	 MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
     if(pRoot == NULL)
     {
         TADD_ERROR(-1,"Not find element=[MDBConfig] when loading table configuration.");
         return -1;
     }

     
	 char strTableDDL[MAX_SQL_LEN] = {0};
	 
	 snprintf(strTableDDL,sizeof(strTableDDL),"create table %s (",psTableName);
	 
     for(MDBXMLElement* pEle=pRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
     {
			std::vector<string> vtColName;
			vtColName.clear();
			CHECK_RET(GetColumnDefine(strTableDDL,pEle,vtColName),"get column define string failed for table %s",psTableName);
			CHECK_RET(GetTablePropertyDefine(strTableDDL,pEle),"get table property define string failed for table %s",psTableName);

			printf("%s\n",strTableDDL);

			CHECK_RET(GetIndexDefine(psTableName,pEle,vtColName),"get table index  define string failed for table %s",psTableName);
			
			break;
			
     }

	 return iRet;

}


int TMdbExptDDLCtrl::ExptSysTablesDDL(bool bOutName)
{
	if(bOutName)
	{
		printf("DBA_TABLES\n"
	        "DBA_COLUMN\n"
	        "DBA_INDEX\n"
	        "DBA_TABLE_SPACE\n"
	        "DBA_SEQUENCE\n"
	        "DBA_USER\n"
	        "DBA_SESSION\n"
	        "DBA_RESOURCE\n"
	        "DBA_SQL\n"
	        "DBA_PROCESS\n");
	}
	else
	{
		printf("                         \n"
				"create table DBA_TABLES(\n"
				"table_name char(32),\n"
				"table_space char(32),\n"
				"record_set_counts number(8),\n"
				"real_counts number(8),\n"
				"stat char(4),\n"
				"expand_record number(8),\n"
				"read_lock char(4),\n"
				"write_lock char(4),\n"
				"column_counts number(8),\n"
				"index_counts number(8),\n"
				"primary_key char(32),\n"
				"cin_counts number(8),\n"
				"left_cin_nodes number(8),\n"
				"full_pages number(8),\n"
				"free_pages number(8),\n"
				"shard_backup char(4),\n"
				"storage_type char(4))record_counts=[1000], rep_type =[norep];\n");
	   

	}

	return 0;

}

//true, 只导出名称;false,导出定义
//遍历存储目录导出表定义
int TMdbExptDDLCtrl::ExptUserTablesDDL(bool bOutName)
{
	int iRet = 0;
	TMdbConfig config;
    char sTabDir[MAX_PATH_NAME_LEN] = {0};
    snprintf(sTabDir, sizeof(sTabDir), "%s.TABLE", sCfgDirName);
        
    if(!TMdbNtcDirOper::IsExist(sTabDir))
    {
         TADD_ERROR(-1,"table dir not exist.");  
         return -1;
     }

     TMdbNtcFileScanner tDirScan;
     if(!tDirScan.ScanDir(sTabDir))
     {   
          TADD_ERROR(-1,"Get Table info failed.");
          return ERR_OS_OPEN_DIR;
     }

     const char* psTabDir = NULL;
     char sTabName[MAX_NAME_LEN] = {0};
	 
     while((psTabDir = tDirScan.GetNext()) != NULL)
     {
           memset(sTabName, 0, sizeof(sTabName));
		   
           if(config.GetTableNameFromDir(psTabDir,sTabName,sizeof(sTabName)) < 0)
           {
                iRet = ERROR_UNKNOWN;
                break;
           }

		   //printf("%s\n","--tables");
		   if(bOutName)
		   {
				printf("%s\n",sTabName);
				continue;

		   	}
           	iRet = ExptOneTableDDL(psTabDir,sTabName);
            if(0 != iRet)
            {
            	TADD_ERROR(iRet,"Get table %s ddl failed ",sTabName);
                return iRet;
            }

           
      }
        
    return iRet;

}

//true， 导出名称;false,导出定义
int TMdbExptDDLCtrl::ExportSequences(bool bOutName)
{
	int iRet = 0;
	
    FILE * m_pSeqFile = NULL;
    
    char sFileName[MAX_NAME_LEN];
    memset(sFileName,0,sizeof(sFileName));
	char *pszHome = getenv("QuickMDB_HOME");

   	SAFESTRCPY(sFileName,sizeof(sFileName),pszHome);
	size_t iLen =strlen(sFileName); 
    if(sFileName[iLen-1] != '/')
    {
        sFileName[iLen] = '/';
		sFileName[iLen+1] = '\0';
    }
	
		
    snprintf(sFileName+strlen(sFileName),sizeof(sFileName)-strlen(sFileName),".data/%s/storage/%s",sDsnName,"Sequence.mdb");
    m_pSeqFile = fopen (sFileName,"rb+");

   
	if (NULL == m_pSeqFile)
	{
       
		return  0;	
	}

	//从文件加载sequence
    char* pBuff = NULL;
    int iBuffSize = MAX_SEQUENCE_COUNTS* sizeof(TMemSeq);
    pBuff = new(std::nothrow) char[iBuffSize];
    if(pBuff == NULL)
    {
        SAFE_CLOSE(m_pSeqFile);
        return -1;
    }

	
    size_t iReadPageCount = fread(pBuff,sizeof(TMemSeq),MAX_SEQUENCE_COUNTS,m_pSeqFile);
    if(iReadPageCount == 0)
    {
       
        SAFE_CLOSE(m_pSeqFile);
        SAFE_DELETE(pBuff);
        return iRet;
    }
	
    TMemSeq  pSeq ;
	printf("Sequence\n");
    for(size_t i = 0; i<iReadPageCount;i++)
    {
        
        memcpy(&pSeq,pBuff+(sizeof(TMemSeq)*i),sizeof(TMemSeq));
		if(bOutName)
			printf("%s\n",pSeq.sSeqName);
		else
		{
			char sSeqDDL[MAX_SQL_LEN] = {0};
			snprintf(sSeqDDL,sizeof(sSeqDDL),"Add sequence (SEQ_NAME('%s'),DSN_NAME('%s'),START_NUMBER(%lld),END_NUMBER(%lld),STEP_NUMBER(%lld),CUR_NUMBER(%lld));",pSeq.sSeqName,sDsnName,pSeq.iStart,pSeq.iEnd,pSeq.iStep,pSeq.iCur);
			printf("%s\n",sSeqDDL);
		}
		
      
		
	}

	
    SAFE_CLOSE(m_pSeqFile);
    SAFE_DELETE(pBuff);
    return iRet;

}


//导出job的定义
int TMdbExptDDLCtrl::ExportJobs(bool bOutName)
{
	 
	   int iRet = 0;
	   char sJobFile[MAX_FILE_NAME] = {0};
	   char sJobSql[MAX_SQL_LEN] = {0};
	   snprintf(sJobFile,sizeof(sJobFile),"%s.JOB/.QuickMDB_JOB.xml",sCfgDirName);
	   if(false == TMdbNtcFileOper::IsExist(sJobFile))
	   {//没有job 文件
		   TADD_FLOW("No job info find.");
		   return 0;
	   }
	   //读取配置文件
	   MDBXMLDocument tDoc(sJobFile);
	   if (false == tDoc.LoadFile())
	   {
		   CHECK_RET(ERR_APP_LAOD_CONFIG_FILE_FALIED,"Load job configuration failed.",sJobFile);
	   }
	   MDBXMLElement* pRoot = tDoc.FirstChildElement("MDB_JOB");
	   CHECK_OBJ(pRoot);
	  
	   MDBXMLElement* pEle = NULL;
	   for(pEle=pRoot->FirstChildElement("job"); pEle; pEle=pEle->NextSiblingElement("job"))
	   {
		   TMdbJob tJob;
		   const char * pValue = NULL;
		   char sJobName[MAX_NAME_LEN] = {0};
		   char sDate[20] = {0};
		   char sInterVal[20] = {0};
		   char sRateType[20] = {0};
		   char sSql[MAX_SQL_LEN] = {0};
		   //name
		   pValue =  pEle->Attribute("name");
		   CHECK_OBJ(pValue);
		   
		   SAFESTRCPY(sJobName,sizeof(tJob.m_sName),pValue);
		   if(bOutName)
		   {
				printf("%s\n",sJobName);
				continue;
		   }
		   
		   //nextdate
		   pValue =  pEle->Attribute("exec_date");
		   CHECK_OBJ(pValue);
		   if(TMdbNtcStrFunc::IsDateTime(pValue) == false)
		   {
			   CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"[%s] is not a date for job define %s",pValue,sJobName);
		   }

		   SAFESTRCPY(sDate,sizeof(sDate),pValue);
		   
		   
		   //interval
		   pValue =  pEle->Attribute("interval");
		   CHECK_OBJ(pValue);
		   //tJob.m_iInterval = TMdbNtcStrFunc::StrToInt(pValue);
		   SAFESTRCPY(sInterVal,sizeof(sInterVal),pValue);
		   
		   //ratetype
		   pValue =  pEle->Attribute("ratetype");
		   CHECK_OBJ(pValue);
		   CHECK_RET(tJob.SetRateType(pValue),"SetRateType faild.");
		   SAFESTRCPY(sRateType,sizeof(sRateType),pValue);
		   
		   //sql
		   pValue =  pEle->Attribute("sql");
		   CHECK_OBJ(pValue);
		   if(0 == pValue[0])
		   {
			   CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"please input sql for job");
		   }
		   
		   SAFESTRCPY(sSql,sizeof(sSql),pValue);

		   //get create job sql
		   snprintf(sJobSql,sizeof(sJobSql),"create job %s(exec_date('%s'),interval(%s),ratetype(%s),sql('%s'));",sJobName,sDate,sInterVal,sRateType,sSql);
		   printf("%s\n",sJobSql);

	   }
	   
	
	   return iRet;

}

//export sql:create database......
int TMdbExptDDLCtrl::ExprtDBDDL(MDBXMLElement* pESys)
{

	MDBXMLAttribute* pAttr		= NULL;
	MDBXMLAttribute* pAttrValue = NULL;
	int iRet = 0;
	char strDDL[MAX_SQL_LEN] = {0};

	snprintf(strDDL, sizeof(strDDL),"Create database %s(\n",sDsnName);
	
	for (MDBXMLElement* pSec=pESys->FirstChildElement("section"); pSec; pSec=pSec->NextSiblingElement("section"))
	{
			pAttr  = pSec->FirstAttribute();
			pAttrValue = pAttr->Next();
			if(pAttrValue != NULL)
			{
				if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0 
					&& TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Name(), "value") == 0)
				{
					
						//pAttr->Value() "-" biancheng"_"
						char strAttrName[MAX_NAME_LEN] = {0};
						strncpy(strAttrName,pAttr->Value(),sizeof(strAttrName)-1);
						for(int i=0; i<strlen(strAttrName); i++)
						if (strAttrName[i] == '-')
								strAttrName[i] = '_';
						snprintf(strDDL+strlen(strDDL), sizeof(strDDL)-strlen(strDDL),"%s('%s'),\n",strAttrName,pAttrValue->Value());
					
				}
			}
				
				

	}

	//最后1个，去掉
	strDDL[strlen(strDDL)-2] = ')';
	strDDL[strlen(strDDL)-1] = ';';
	printf("%s\n",strDDL);
	return iRet;				

}

//export sql: connect to oracle/mysql 'XXX/XXX@XXX'
int TMdbExptDDLCtrl::ExptDataSourceDDL(MDBXMLElement* pEle)
{
	int iRet = 0;
	char strDDL[MAX_SQL_LEN] = {0};

	snprintf(strDDL, sizeof(strDDL),"Connect to");
	MDBXMLElement* pEDS = pEle->FirstChildElement("DataSource");
	if(NULL == pEDS)
	{
		TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"DataSource node does not exist,please check the DSN configuration.");
		return ERR_APP_CONFIG_ITEM_NOT_EXIST;
	}
	MDBXMLAttribute* pAttr		= NULL;
	for(pAttr=pEDS->FirstAttribute(); pAttr; pAttr=pAttr->Next())
	{
		if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
		{
			
		}
		else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "type") == 0)
		{
			if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "oracle") == 0)
			{
				
				snprintf(strDDL+strlen(strDDL), sizeof(strDDL)-strlen(strDDL)," oracle");
			}
			else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "mysql") == 0)
			{
				
				snprintf(strDDL+strlen(strDDL), sizeof(strDDL)-strlen(strDDL)," mysql");
			}
			else
			{
				TADD_ERROR(ERROR_UNKNOWN,"Invalid DS-Type=[%s].",pAttr->Value());
				return -1;
			}
		}
		else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "user") == 0)
		{
			
			snprintf(strDDL+strlen(strDDL), sizeof(strDDL)-strlen(strDDL)," '%s",pAttr->Value());
		}
		else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "password") == 0)
		{
			if(!TMdbEncrypt::IsEncryptStr(pAttr->Value()))
			{
				
				snprintf(strDDL+strlen(strDDL), sizeof(strDDL)-strlen(strDDL),"/%s",pAttr->Value());
			}
			else
			{	char strPwd[MAX_NAME_LEN] = {0};
				TMdbEncrypt::DecryptEx(const_cast<char *>(pAttr->Value()),strPwd);
				snprintf(strDDL+strlen(strDDL), sizeof(strDDL)-strlen(strDDL),"/%s",strPwd);
			}
		}
		else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "db-id") == 0)
		{
					
			snprintf(strDDL+strlen(strDDL), sizeof(strDDL)-strlen(strDDL),"@%s';",pAttr->Value());
		}
		else
		{
			TADD_ERROR(ERROR_UNKNOWN,"Invalid element=[%s].", pAttr->Name());
			return -1;
		}
	}

	printf("%s\n",strDDL);
	return iRet;

}

//export sql:create table space ......
int TMdbExptDDLCtrl::ExptTblSpcDDL(MDBXMLElement* pMDB)
{
	int iRet = 0;
	char strDDL[MAX_SQL_LEN] = {0};
	snprintf(strDDL, sizeof(strDDL),"Create tablespace ");
	
	 for(MDBXMLElement* pEle=pMDB->FirstChildElement("table-space"); pEle; pEle=pEle->NextSiblingElement("table-space"))
        {
            MDBXMLAttribute* pAttr = NULL;
           
          
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                   
                    snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL),"%s",pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "page-size") == 0)
                {
                    
                   snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL)," pagesize %s",pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ask-pages") == 0)
                {
                   
                    snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL)," askpage %s",pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "is-file-storage") == 0)
                {
                    snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL)," storage '%s';\n",pAttr->Value());
                }
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].", pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }
            
            
           printf("%s;\n",strDDL);
		   memset(strDDL,0,sizeof(strDDL));
		   snprintf(strDDL, sizeof(strDDL),"Create tablespace ");
        }

	 return iRet;

}

//export sql:create user ......
int TMdbExptDDLCtrl::ExptUserDDL(MDBXMLElement* pMDB)
{
	char strDDL[MAX_SQL_LEN] = {0};
	snprintf(strDDL, sizeof(strDDL),"Create user ");
	char strPwd[MAX_SQL_LEN] = {0};
	   for(MDBXMLElement* pEle=pMDB->FirstChildElement("user"); pEle; pEle=pEle->NextSiblingElement("user"))
	   {
		   MDBXMLAttribute* pAttr = NULL;
		   
		   for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
		   {
			   if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
			   {
				 	snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL)," %s",pAttr->Value());
			   }
			   else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "password") == 0)
			   {
				   if(!TMdbEncrypt::IsEncryptStr(pAttr->Value()))
				   {
					  snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL)," identified by '%s'",pAttr->Value());
				   }
				   else
				   {
				   		memset(strPwd,0,sizeof(strPwd));
					   	TMdbEncrypt::DecryptEx(const_cast<char *>(pAttr->Value()),strPwd);
						snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL)," identified by '%s'",strPwd);
				   }
			   }
			   else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "access") == 0)
			   {

			   	   snprintf(strDDL+strlen(strDDL),sizeof(strDDL)-strlen(strDDL)," accessed by '%s'",pAttr->Value());
				  
				  
			   }
			   else
			   {
				   TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].", pAttr->Name());
				   return ERR_APP_INVALID_PARAM;
			   }
		   }

		
		   
		   printf("%s;\n",strDDL);
		   
		   memset(strDDL,0,sizeof(strDDL));
		   snprintf(strDDL, sizeof(strDDL),"Create user ");
		 
	   }
	   
	   return 0;
}


//导出数据库的定义信息 db,datasource,user,tablspace
int TMdbExptDDLCtrl::ExptDatabaseDDL()
{
	int iRet = 0;
	TMdbConfig cfg;
	CHECK_RET(cfg.GetFullPathCfgName(sDsnName,sCfgFileName),"get config file for dsn %s failed",sDsnName);
	
	MDBXMLDocument tDoc(sCfgFileName);
     if (false == tDoc.LoadFile())
      {
           TADD_ERROR(ERROR_UNKNOWN,"Load sys configuration failed.");
           return -1;
      }
      MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
      if(NULL == pRoot)
      {
          TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"MDBConfig node does not exist in sys configuration.");
          return ERR_APP_CONFIG_ITEM_NOT_EXIST;
      }

	  MDBXMLElement* pESys = pRoot->FirstChildElement("sys");
      if(NULL == pESys)
      {
          TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"sys node does not exist,please check the DSN configuration files.");
          return ERR_APP_CONFIG_ITEM_NOT_EXIST;
      }

	//DB DDL
	CHECK_RET(ExprtDBDDL(pESys),"export database ddl failed");
	//connect oracle or mysql ddl
	CHECK_RET(ExptDataSourceDDL(pRoot),"export datasource ddl failed");
	//user ddl
	CHECK_RET(ExptUserDDL(pRoot),"export user ddl failed");
	//tablespace ddl
	CHECK_RET(ExptTblSpcDDL(pRoot),"export tablespace ddl failed");
	return iRet;
	
}



	
//}

