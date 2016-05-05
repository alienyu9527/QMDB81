/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepLoadCtrl.h		
*@Description: 分片备份的数据上载模块
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_LOAD_DATA_CTRL_H__
#define __ZTE_MINI_DATABASE_REP_LOAD_DATA_CTRL_H__
#include "Interface/mdbQuery.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepNTC.h"


//namespace QuickMDB
//{
    class TMdbRepLoadThread:public /*QuickMDB::*/TMdbNtcThread
    {
    public:
        TMdbRepLoadThread(TMdbRepDataClient *pClient=NULL);
        ~TMdbRepLoadThread();

    public:
        int AddRoutingID(int iRoutingID);//增加需要加载的路由ID
        int GetHostID();//获取对应的备机ID
        int SetRuningInfo(TMdbConfig* pMdbCfg);//设置线程运行时信息

    private:
        int LoadDataFromRepHost(const char*sRoutinglist);//从备机加载数据
        int LoadDataFromStorage(const char* sRoutinglist);//从其他存储加载数据

    protected:
        virtual int Execute();

    private:
        int m_iLocHostID;
        int m_iRepHostID;
        TMdbConfig *m_pMdbCfg;
        std::vector<int> m_aiRoutingID;
        TMdbRepDataClient *m_pClient;//备机对应的连接，从存储加载时，其值为NULL             
    };

    

    class TMdbRepLoadDataCtrl
    {
    public:
        TMdbRepLoadDataCtrl();
        ~TMdbRepLoadDataCtrl();

    public:
        int Init(const char* sDsn, TMdbConfig* pMdbCfg);

        // 与配置服务端连接状态下加载数据
        int LoadData();
        // 与配置服务端未连接状态下加载数据
        int LoadDataNoSvr();

    private:
        int ConnectRepHosts();//创建从所有备机加载数据的线程
        int DisConnectRepHosts();//断开与所有备机的连接
        int DealLeftFile();//处理本机遗留数据
        int LoadDataFromRep();//从备机加载数据

        int CreateLoadThreads();//为每个路由创建或者分配加载的线程

    private:
        bool IsFileOutOfDate(time_t tTime);//文件是否过期
        TMdbRepDataClient * GetClient(int iHostID);//获取主机对应的连接
        TMdbRepLoadThread *GetThread(int iHostID);//获取主机对应的加载线程

        int LoadDataFromOracle(int iRouting_ID);//从oracle加载数据
        int LoadDataFromFile(int iRouting_ID);//从文件加载数据
        int LoadDataFromMySql(int iRouting_ID);//从MySql中加载数据

        // 从本地配置文件读取分片备份配置信息到共享内存中
        int WriteLocalRoutingInfo();
        // 将共享内存中最新的分片备份配置信息保存到本地配置文件
        int SyncLocalConfig();

    private:
        TMdbRepConfig *m_pRepConfig;//配置服务相关配置
        TMdbShmRepMgr *m_pShmMgr;//共享内存管理类指针
        TMdbRepMonitorClient *m_pMonitorClient;       
        const TMdbShmRep *m_pShmRep;//路由信息共享内存

        TMdbRepDataClient **m_arpLoadClient;
        /*QuickMDB::*/TMdbNtcAutoArray m_arThread;
         std::string m_strDsn;//dsn名称

         TMdbConfig* m_pMdbCfg;

         bool m_bSvrConnFlag; // 是否连接上分片备份配置服务端

         int m_iHostID; //本机ID
       
    };

    

//};
#endif