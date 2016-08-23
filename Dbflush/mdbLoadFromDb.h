/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbLoadFromDb.h		
*@Description： 负责从Oracle上载数据；
*@Author:		li.shugang
*@Date：	    2008年12月15日
*@History:
******************************************************************************************/
#ifndef __MDB_LOAD_FROM_DB_H__
#define __MDB_LOAD_FROM_DB_H__

#include <vector>
#ifndef WIN32
#include <pthread.h>
#endif


#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Interface/mdbQuery.h"
#include "Dbflush/mdbDAOLoad.h"


using namespace std;

//namespace QuickMDB{

    class TMdbShmDSN;

    class TMdbLoadFromDb
    {
    public:
        TMdbLoadFromDb();
        ~TMdbLoadFromDb();

        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  根据配置文件分配表空间  
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Init(TMdbConfig *m_pConfig);

        #ifdef WIN32
        static DWORD WINAPI agent( void* p ); 
        #else
        static void* agent(void* p);
        #endif
        
        int svc(const char* pTSName);
        /******************************************************************************
        * 函数名称	:  LoadAll()
        * 函数描述	:  一个个的把表数据上载上来  
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadAll();

        /******************************************************************************
        * 函数名称	:  LoadTable()
        * 函数描述	:  把一个表的数据上载上来  
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTable(TMdbDatabase* pMdb, TMdbTable* pTable,bool bIsNeedLoad = true);

    private:    
        bool FilterTable(TMdbTable* pTable, const char* psTSName, const char* psTabName);

    private:
        TMdbConfig *m_pConfig;
        TMdbShmDSN *m_pShmDSN;

        #ifndef WIN32
        vector<pthread_t> *m_vPthreadT;
        #else
        vector<HANDLE> *m_vThreadHandle;
        #endif

    };

    struct PthreadParam
    {
        TMdbLoadFromDb *tmdbLoadfromOra;
        char m_sTSName[MAX_NAME_LEN];
        //int             iTablespaceId;

        PthreadParam()
        {
            tmdbLoadfromOra = NULL;
            //iTablespaceId = 0;
            memset(m_sTSName, 0, sizeof(m_sTSName));
        }
        ~PthreadParam()
        {
            tmdbLoadfromOra = NULL;
            memset(m_sTSName, 0, sizeof(m_sTSName));
        }
    };


    class TMdbReLoadFromOra
    {
    public:
        TMdbReLoadFromOra();
        ~TMdbReLoadFromOra();

        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  根据配置文件分配表空间  
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Init(char* pszDsn);
        
        /******************************************************************************
        * 函数名称	:  LoadAll()
        * 函数描述	:  一个个的把表数据上载上来  
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadAll(const char* pszTable, const char * pszFilterSql=NULL);    
        int LoadTables(const char* pszTable, const char * pszFilterSql,TMdbDatabase *mdb,TMdbDAOLoad *dao);
    private:    
        bool FilterTable(TMdbTable* pTable, const char* psTabName);

    private:
        TMdbConfig m_tConfig;;
        TMdbShmDSN *m_pShmDSN;
    };


//}


#endif 
