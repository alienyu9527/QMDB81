#ifndef _MDB_EXPT_DDL_CTRL_H_
#define _MDB_EXPT_DDL_CTRL_H_

#include <string>
#include <vector>
#include <map>
#include <ctype.h>
#include "Interface/mdbQuery.h"
#include "Helper/mdbStruct.h"
//#include "Helper/mdbFileList.h"
//#include "Helper/mdbFileOper.h"
//#include "Helper/TOraDBQuery.h"
#include "Helper/TDBFactory.h"
#include "Helper/mdbDictionary.h"
#include "Helper/mdbXML.h"

//namespace QuickMDB{








class TMdbExptDDLCtrl
{
public:
	TMdbExptDDLCtrl();
	~TMdbExptDDLCtrl();
	
	int  Init(const char*pDsn);
	int  GetConfigHomePath();
	int  ExptDatabaseDDL();
	
	
	int  ExprtDBDDL(MDBXMLElement* pESys);
	int  ExptDataSourceDDL(MDBXMLElement* pEle);
	int  ExptUserDDL(MDBXMLElement* pMDB);
	int  ExptUserTablesDDL(bool bOutName=false);
	int  ExptSysTablesDDL(bool bOutName=false);
	
	
	int  ExportSequences(bool bOutName=false);
	int  ExportJobs(bool bOutName=false);
	int  ExptTblSpcDDL(MDBXMLElement* pMDB);
	char sCfgFileName[MAX_PATH_NAME_LEN];
	char sDsnName[MAX_NAME_LEN];
	char sCfgDirName[MAX_PATH_NAME_LEN];
private:
	int GetTablePropertyDefine(char* psTableDDL,MDBXMLElement* pMDB);
	int  GetColumnDefine(char* psTableDDL,MDBXMLElement* pMDB,std::vector<string> & vecColName);
	int  GetIndexDefine(const char* psTableName,MDBXMLElement* pMDB,std::vector<string> & vecColName);
	int  ExptOneTableDDL(const char* psTableDir, const char* psTableName);
};
//}
#endif

