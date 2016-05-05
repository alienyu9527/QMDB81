#include "Helper/TDBFactory.h"
//--HH--#include "Helper/mdbStruct.h"
#include "Helper/TThreadLog.h"

//namespace QuickMDB{

TMDBDBInterface* TMDBDBFactory::CeatDB()
{
	#ifdef DB_ORACLE
        return new TOraDBDatabase();    
	#elif DB_MYSQL    
        return new TMsqDBDatabase();
    #else
        TADD_ERROR(ERROR_UNKNOWN, "[%s:%d] TMDBDBInterface creator()  DB_TYPE is wrong, check Makefile.incl",__FILE__,__LINE__); 
        return NULL;
	#endif    
}

//}


