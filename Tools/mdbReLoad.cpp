
#include <string>
#include <iostream>
#include "Helper/TThreadLog.h"
#include "Dbflush/mdbLoadFromDb.h"

//using namespace QuickMDB;

int main(int argc, char* argv[])
{

    if(argc < 3)
    {
        printf("-------\n"
    		" Usage:\n"
    		"   <DSN> [table|all] [filer-sql]\n"
    		" Example\n" 
        	"   mdbReLoad ocs bal\n"  
        	"   mdbReLoad ocs all\n" 
        	"   mdbReLoad ocs bal  int_1>0\n" 
        	" Note\n"
        	"      DSN :data source .\n"
        	"      table : table name OR all .    \n"
    		"-------\n" );
        return -1;
    }

    TADD_START(argv[1],"mdbReLoad",  0, true,false);
    TMdbReLoadFromOra Load;
    int iRet = Load.Init(argv[1]);
    if(iRet != 0)
    {
        TADD_ERROR(-1,"[%s : %d] : mdbReLoad:: Can't dispath table-space.", __FILE__, __LINE__);
        return -1;    
    }

	if(argc == 3)
    	iRet = Load.LoadAll(argv[2]);
	else
		iRet = Load.LoadAll(argv[2],argv[3]);
    if(iRet != 0)
    {
        TADD_ERROR(-1,"[%s : %d] : mdbReLoad::Can't load some tables.", __FILE__, __LINE__);
        return -1;    
    }
    TADD_DETAIL("mdbReLoad Finished");
    return 0;
}
