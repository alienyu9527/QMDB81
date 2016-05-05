/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbDataExport.h     
*@Description： 内存表的数据导出的访问接口
*@Author:       li.shugang
*@Date：        2009年10月30日
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_DATA_EXPORT_H__
#define __QUICK_MEMORY_DATABASE_DATA_EXPORT_H__

#include <string>
#include <iostream>

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Interface/mdbQuery.h"
#include "Dbflush/mdbDAOBase.h"


//namespace QuickMDB{

    class TMdbShmDSN;
    class TMdbTableCtrl;



    class TMdbDataExport
    {
    public:
        TMdbDataExport();
        ~TMdbDataExport();
        
    public:
        /******************************************************************************
        * 函数名称  :  Login()
        * 函数描述  :  根据登录信息，登录内存/Oracle数据库  
        * 输入      :  pszInf, 内存数据库的DSN名称
        * 输出      :  无
        * 返回值    :  成功返回0，否则返回-1
        * 作者      :  li.shugang
        *******************************************************************************/
        int Login(const char* pszInf);
        
        /******************************************************************************
        * 函数名称  :  Export()
        * 函数描述  :  导出数据  
        * 输入      :  pszTableName, 需要导出的表   
        * 输入      :  pszObjTable,  导出的目标表   
        * 输入      :  pszWhere,     查询条件 
        * 输出      :  无
        * 返回值    :  成功返回导出的记录数，否则返回-1
        * 作者      :  li.shugang
        *******************************************************************************/
        int Export(const char* pszTableName, const char* pszObjTable,const char* pszWhere="");
		int ForceExport(const char* pszTableName, const char* pszObjTable,const char* pszWhere="");
        
    private:
    	//获取mdb用户信息
        int GetMDBUser(const char* pszDSN, char* pszUID, char* pszPWD);
    	//获取oracle用户信息
        int GetOraUser(char* pszDSN, char* pszUID, char* pszPWD);
    	//检查表同步属性、拼写查询SQL
        int CheckMDBTable(const char* pszTableName);
    	//拼写Oracle insert SQL
        int CheckOraTable(const char* pszTableName);
		int ForceCheckOraTable(const char* pszTableName);
        
    private:
        TMdbDatabase m_tDB;     
        TMDBDBInterface* m_pDBLink;   //链接
        TMdbConfig *m_pConfig;  
        char m_sMDBSQL[MAX_SQL_LEN]; //对应的SQL
        char m_sORaSQL[MAX_SQL_LEN]; //对应的SQL    
        char *m_pCountSQL;
        TMdbDAOBase* m_pDAO;
        TMdbTable* m_pTable;

    	string sWhere;
    };

//}


#endif //__QUICK_MEMORY_DATABASE_DATA_EXPORT_H__




