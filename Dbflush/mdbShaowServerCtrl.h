/****************************************************************************************
*@Copyrights  2011，中兴软创（南京）计算机有限公司 开发部 CCB项目--QMDB团队
*@                   All rights reserved.
*@Name：        mdbShaowServerCtrl.h
*@Description： 影子表独立部署下的进程监控处理
*@Author:       li.ming
*@Date：        2014年4月17日
*@History:
******************************************************************************************/

#ifndef _QMDB_SHADOW_SERVER_CTRL_H_
#define _QMDB_SHADOW_SERVER_CTRL_H_

#include "Helper/mdbStruct.h"
#include "Helper/mdbShmSTL.h"

//namespace QuickMDB{


    class TShmMonitorInfo
    {
    public:
        TShmMonitorInfo();
        ~TShmMonitorInfo();

        void Clear();

    public:
        bool m_bRunFlag; // true - run; false - stop;
        int m_iProcId; // process id
    };

    class TShadowCfgNode
    {
    public:
        TShadowCfgNode();
        ~TShadowCfgNode();

        void Clear();

    public:
        //int m_iMdbDsnId;
        long long m_llMdbDsnId;
        char m_sMdbDsn[MAX_NAME_LEN];
        char m_sDBType[MAX_NAME_LEN];
        char m_sDBUid[MAX_NAME_LEN];
        char m_sDBPwd[MAX_NAME_LEN];
        char m_sDBTns[MAX_NAME_LEN];
    };

    class TMdbShadowSvrCtrl
    {
    public:
        TMdbShadowSvrCtrl();
        ~TMdbShadowSvrCtrl();

        int Start(const char* psDsn);
        int Stop(const char* psDsn);
        int Monitor();

        TShmMonitorInfo* AttachSvrMgr(long long llDsnId);
         int CheckDsnCfg(const char* psDsn, TShadowCfgNode& tCfgNode);

    private:        
       
        int StartProc(const char* psDsn);

    private:
        int m_iShmId;
        TShmMonitorInfo* m_pShmInfo;
        TShmAlloc m_tShmAlloc;
        char m_sDsn[MAX_NAME_LEN];
        bool m_bAttach;
        
    };

//}

#endif

