/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepCtrl.h		
*@Description: 分片备份主进程管理类，负责与配置服务通信和数据上载
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_CTRL_H__
#define __ZTE_MINI_DATABASE_REP_CTRL_H__
#include "Interface/mdbQuery.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepNTC.h"

#include "Control/mdbProcCtrl.h"

//namespace QuickMDB
//{
    class TMdbRepCtrl
    {
    public:
        TMdbRepCtrl();
        ~TMdbRepCtrl();

        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  初始化
        * 输入		:  sDsn mdb的Dsn名称
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(const char* sDsn);

        /******************************************************************************
        * 函数名称	:  LoadData()
        * 函数描述	:  加载数据, 供测试程序使用，通过设定不同的port，同一台主机可以启动多个qmdb
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  jiang.lili
        *******************************************************************************/
        // int LoadData(const char* sDsn, int iPort);

        /******************************************************************************
        * 函数名称	:  LoadData()
        * 函数描述	:  加载数据
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  jiang.lili
        *******************************************************************************/
        // int LoadData(const char* sDsn, TMdbConfig* pMdbCfg);

        /******************************************************************************
        * 函数名称	:  Start()
        * 函数描述	:  启动分片备份各个子进程，进行数据同步、状态上报、路由变更处理
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Start();

         /******************************************************************************
        * 函数名称	:  ReportState()
        * 函数描述	:  上报QuickMDB状态，供QuickMDB调用，不需要调用Init函数，直接调用即可
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  jiang.lili
        *******************************************************************************/
        static int ReportState(const char*sDsn,int iHostID, EMdbRepState eState);
    private:
        int ConnectMDB();//连接MDB
        int ConnectServer();//连接配置服务
        //int StartRepProcess();//启动同步进程
        //int MonitorProcess();//监控进程的运行
        int DealServerCmd(TRepNTCMsg& tCmd);//处理来自配置服务的命令
    private:
        char m_sDsn[MAX_NAME_LEN];//Dsn名称
        TMdbDatabase *m_pMDB;//数据库连接指针
        
        TMdbRepConfig *m_pRepConfig;//配置文件
        TMdbRepMonitorClient* m_pMonitorClient;//状态监控客户端    
        TMdbShmRepMgr *m_pShmMgr;//共享内存管理类

        TMdbProcCtrl m_tProcCtrl;
        bool m_bSvrConn; // 配置服务端是否连接
    };

	class TMdbShardBuckupCfgCtrl
	{
	public:
		TMdbShardBuckupCfgCtrl();
		~TMdbShardBuckupCfgCtrl();
		int Init(const char * pszDSN);
		int GetShardBuckupInfo();
	public:
		const char * m_sDsn;
		TMdbRepConfig * m_pRepConfig;
		bool m_bSvrConnFlag;
		TMdbRepMonitorClient * m_pMonitorClient;
		TMdbShmRepMgr * m_pShmMgr;
	};
	
//}
#endif
