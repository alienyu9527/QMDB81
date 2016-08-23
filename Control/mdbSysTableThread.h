/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbTimeThread.h
*@Description�� �������ʱ����Ϣ
*@Author:		li.shugang
*@Date��	    2009��09��12��
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_SYSTEM_TABLE_THREAD_H__
#define __ZX_MINI_DATABASE_SYSTEM_TABLE_THREAD_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Helper/TThreadLog.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbLinkCtrl.h"
#include "Control/mdbProcCtrl.h"

#ifdef WIN32
#include <windows.h>
#endif

 //   namespace QuickMDB{

    //ϵͳ��
    enum E_SYS_TABLE
    {
        E_DBA_TABLE       = 0,
        E_DBA_TABLESPACE = 1,
        E_DBA_SEQUENCE= 2,
        E_DBA_SESSION = 3,
        E_DBA_RESOURCE= 4,
        E_DBA_COLUMN   = 5,
        E_DBA_INDEX    = 6,
        E_DBA_PROCESS  = 7,
        E_DBA_MAX
    };

    enum E_QUERY_TYPE
    {
        E_INSERT  = 0,
        E_DELETE  = 1,
        E_UPDATE = 2,
        E_SELECT  = 3,
        E_SELECT_ALL = 4,
        E_QUERY_MAX
    };
    //��Դ��Ϣ
    struct ST_RESOURCE_INFO
    {
        long long llMemKey;
        SHAMEM_T iMemId;
        char sMemType[MAX_NAME_LEN];
        size_t llMemSize;
        size_t llMemLeft;
        void Clear()
        {
            llMemKey = 0;
            iMemId   = INITVAl;
            sMemType[0] = 0;
            llMemSize = 0;
            llMemLeft  = 0;
        }
    };

    //ϵͳ��ͬ��
    class TMdbSysTableSync
    {
    public:
        TMdbSysTableSync();
        ~TMdbSysTableSync();
        int Init(const char * sDsn);
        int SyncAll();
        void Clear();
    private:
        int SyncTables(TMdbQuery * pQuery[]);
        int SyncTableSpace(TMdbQuery * pQuery[]);
        int SyncSequence(TMdbQuery * pQuery[]);
        int SyncSession(TMdbQuery * pQuery[]);
        int SyncResource(TMdbQuery * pQuery[]);
        int SyncColumn(TMdbQuery * pQuery[]);
        int SyncIndex(TMdbQuery * pQuery[]);
        int SyncProcess(TMdbQuery * pQuery[]);

        int SyncDataAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
        int GeneralSyncResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stReourceInfo);
        int SyncBaseIdxAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
        int SyncConflictIdxAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
        int SyncVarcharAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);

		int SyncMhashBaseIdxAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
		int SyncMhashConflictIdxMgrAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
		int SyncMhashConflictIdxAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
		int SyncMhashLayerIdxMgrAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
		int SyncMhashLayerIdxAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
		int SyncMhashMutexAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);

	
        int SyncSAResouce(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);//ͬ��[ͬ����]��Ϣ
        int SyncSBBAResouce(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo);
    private:
        int SyncLocalLink(TMdbQuery * pQuery[]);//ͬ����������
        int SyncRemoteLink(TMdbQuery * pQuery[]);//ͬ��Զ������
    private:
        TMdbDatabase * m_pMDB;//mdb ����
        TMdbShmDSN * m_pShmDSN;
        TMdbConfig *m_pMdbConfig;
         TMdbDSN    * m_pDsn;

        TMdbQuery * m_pIDUSQuery[E_DBA_MAX][E_QUERY_MAX];//insert delete update select Query;
        std::map<std::string, int> m_tTableVersion;
        std::map<std::string, int> m_iIndexCounts;
    };

    //ϵͳ��ͬ���߳�
    class TMdbSysTableThread
    {
    public:
        TMdbSysTableThread();
        ~TMdbSysTableThread();
        void Init(TMdbShmDSN* pShmDSN);
        int Start();
        void Stop();  //ֹͣ����
        bool IsRun()
        {
            return m_bIsRun;
        }
    private:
        int svc();
        static void* agent(void* p);
        void ResetSysLogLevel();//������־����
        void ClearLink();//�������Ҫ������
        //void ReleaseMutex();//�ͷų�ʱ��
       // int DiffMSecond(struct timeval tTV1, struct timeval tTV2);
    private:
        bool  m_bRunFlag;
        bool m_bIsRun;
        TMdbShmDSN* m_pShmDSN;
        TMdbDSN    *m_pDsn;
        TMdbLinkCtrl m_tLinkCtrl;
        TMdbProcCtrl m_tProcCtrl;
        time_t m_tLastTime;
    };
//}

#endif //__ZX_MINI_DATABASE_SYSTEM_TABLE_THREAD_H__

