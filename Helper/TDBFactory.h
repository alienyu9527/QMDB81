#ifndef __DATABASE_FACTORY_H__
#define __DATABASE_FACTORY_H__

#ifdef DB_MYSQL
#include "Helper/TMsqDBQuery.h"
#elif  DB_ORACLE
#include "Helper/TOraDBQuery.h"
#elif  DB_NODB
#include "Helper/mdbDBInterface.h"

#endif

//--HH--#include "Helper/mdbConfig.h"

//namespace QuickMDB{

//--HH--enum DB_TYPE
//--HH--{ 
//--HH--    DB_TYPE_ORACLE,
//--HH--    DB_TYPE_MYSQL 
//--HH--};


class TMDBDBFactory
{
public:
    //--HH--static TDBInterface* CeatDB(char dbtype);
    static TMDBDBInterface* CeatDB();
};

//}

#endif   //__DATABASE_FACTORY_H__

