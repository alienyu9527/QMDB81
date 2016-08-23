/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbLoadFromDb.h		
*@Description�� �����Oracle�������ݣ�
*@Author:		li.shugang
*@Date��	    2008��12��15��
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
        * ��������	:  Init()
        * ��������	:  ���������ļ������ռ�  
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Init(TMdbConfig *m_pConfig);

        #ifdef WIN32
        static DWORD WINAPI agent( void* p ); 
        #else
        static void* agent(void* p);
        #endif
        
        int svc(const char* pTSName);
        /******************************************************************************
        * ��������	:  LoadAll()
        * ��������	:  һ�����İѱ�������������  
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadAll();

        /******************************************************************************
        * ��������	:  LoadTable()
        * ��������	:  ��һ�����������������  
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
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
        * ��������	:  Init()
        * ��������	:  ���������ļ������ռ�  
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Init(char* pszDsn);
        
        /******************************************************************************
        * ��������	:  LoadAll()
        * ��������	:  һ�����İѱ�������������  
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
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
