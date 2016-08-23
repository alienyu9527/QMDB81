/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbRepClientCtrl.h	
*@Description： 负责分片备份同步数据的发送
*@Author:		jiang.lili
*@Date：	    2014/03/20
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_REP_CLIENT_CTRL_T_H__
#define __ZX_MINI_DATABASE_REP_CLIENT_CTRL_T_H__

#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepNTC.h"
#include "Control/mdbProcCtrl.h"
#include "Helper/mdbQueue.h"
#include "Replication/mdbRepLog.h"
#include "Control/mdbStorageEngine.h"

//namespace QuickMDB
//{
    /**
    * @brief 客户端处理线程类，对应一台备机
    * 
    */
    class TRepClientEngine: public TMdbNtcThread
    {
    public:
        TRepClientEngine();
        ~TRepClientEngine();
    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(const TMdbShmRepHost &tHost, const char* sFilePath, time_t tFileInvalidTime, const char* sDsn);

        /******************************************************************************
        * 函数名称	:  GetHostID
        * 函数描述	:  获取本线程对应的主机标识
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetHostID()
        {
            return m_iHostID;
        }
		
		int KillRepDataClient();
    protected:
       /******************************************************************************
        * 函数名称	:  Execute
        * 函数描述	:  线程执行函数
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        virtual int  Execute();//线程的执行函数
    private:
		int UpdateRepMode(bool bRollback = false);//更新当前同步模式
		int UpdateOnlineRepMode(bool bRollback = false);
        /******************************************************************************
        * 函数名称	:  CheckRepFile
        * 函数描述	:  检查同步文件是否存在
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int CheckRepFile();
		/******************************************************************************
		* 函数名称	:  CheckShardBakBuf
		* 函数描述	:  检查链路缓存使用情况
		* 输入		:  
		* 输出		:  
		* 返回值	:  0 - 成功!0 -失败
		* 作者		:  jiang.lili
		*******************************************************************************/
		int CheckShardBakBuf();
		/******************************************************************************
		* 函数名称	:  CheckOverdueRepFile
		* 函数描述	:  检查同步文件是否过期，删除过期同步文件
		* 输入		:  
		* 输出		:  
		* 返回值	:  0 - 成功!0 -失败
		* 作者		:  jiang.lili
		*******************************************************************************/
		int CheckOverdueRepFile();

        /******************************************************************************
        * 函数名称	:  SendRepFile
        * 函数描述	:  发送同步文件，无同步文件，则sleep 1s
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SendRepFile();

		/******************************************************************************
		* 函数名称	:  SendRepData
		* 函数描述	:  发送同步记录，无同步文件，则sleep 10ms
		* 输入		:  
		* 输出		:  
		* 返回值	:  0 - 成功!0 -失败
		* 作者		:  jiang.xiaolong
		*******************************************************************************/
		int SendRepData();
        /******************************************************************************
        * 函数名称	:  DoServerCmd
        * 函数描述	:  响应server端命令，目前仅响应心跳检查包
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int DoServerCmd(TMdbMsgInfo* pMsg);
    private:
        int m_iHostID;//主机标识
        TMdbNtcString m_strIP;//IP
        int m_iPort;//Port
		bool m_bSameEndian;//字节序是否一致
        TMdbNtcFileScanner m_tFileScanner;
		TMdbShmDSN * m_pShmDSN;
  
        TMdbNtcString m_strPath;//同步文件的目录
        time_t m_iFileInvalidTime;//同步文件的过期时间
        TMdbRepDataClient *m_pClient;//发送客户端     
        TMdbShmRepMgr* m_pShmMgr;
        TMdbShmRep* m_pShmRep;//共享内存指针
        bool m_bRepFileExist;//是否有对应的同步文件存在
        bool m_bShardBakBufFree;//链路缓存有足够空闲
        TMdbDSN * m_pMdbDSN;//是否启用飞行模式同步
        TMdbOnlineRepQueue m_pOnlineRepQueueCtrl;
    };


	class TMdbWriteRepLog:public TMdbNtcThread
	{
	public:
		TMdbWriteRepLog();
		~TMdbWriteRepLog();
		int Init(int iHostID, const char* sDsn, const char* sFilePath);
		int UpdateRepMode(bool bRollback = false);//更新当前同步模式
		int UpdateOnlineRepMode(bool bRollback = false);
        /******************************************************************************
        * 函数名称	:  CheckRepFile
        * 函数描述	:  检查同步文件是否存在
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int CheckRepFile();
		/******************************************************************************
		* 函数名称	:  CheckShardBakBuf
		* 函数描述	:  检查链路缓存使用情况
		* 输入		:  
		* 输出		:  
		* 返回值	:  0 - 成功!0 -失败
		* 作者		:  jiang.lili
		*******************************************************************************/
		int CheckShardBakBuf();
        int  Execute();
        void Cleanup();
	private:
		int m_iHostID;
		TMdbOnlineRepQueue m_tOnlineRepQueueCtrl;//内存缓冲管理器   
        TMdbShmRepMgr* m_pShmMgr;
        TMdbShmRep* m_pShmRep;//共享内存指针
        TMdbShmDSN * m_pShmDsn;
        TMdbDSN * m_pDsn;
        TRoutingRepLog m_mdbReplog;
        TMdbRedoLogParser m_LogParser;
        TMdbConfig  *m_pConfig;
        bool m_bRepFileExist;//是否有对应的同步文件存在
        bool m_bShardBakBufFree;//链路缓存有足够空闲
        TMdbNtcFileScanner m_tFileScanner;
		TMdbNtcString m_strPath;//同步文件的目录
	};


    /**
    * @brief 分片备份同步客户端
    * 
    */
    class TRepClient
    {
    public:
        TRepClient();
        ~TRepClient();
       /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(const char *sDsn, int iHostID);
       /******************************************************************************
        * 函数名称	:  Start
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Start();
       /******************************************************************************
        * 函数名称	:  DealRoutingChange
        * 函数描述	:  处理路由变更命令
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int DealRoutingChange();//处理路由变更
        /******************************************************************************
        * 函数名称	:  WaitAllThreadQuit
        * 函数描述	:  等待所有线程结束
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int WaitAllThreadQuit();

		/******************************************************************************
        * 函数名称	:  CheckThreadsHeartBeat
        * 函数描述	:  检查线程心跳
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  yu.lianxiang
        *******************************************************************************/
        int CheckThreadHeart(); 

    private:
        TMdbShmRepMgr *m_pShmMgr;//共享内存管理类指针
        TMdbRepConfig *m_pRepConfig;//分片备份配置文件
        TMdbNtcAutoArray m_arThreads;//所有发送同步数据的子线程
        char m_sDsn[MAX_NAME_LEN];
		int m_iHostID;
		TRepClientEngine * m_pClientEngine;
		TMdbWriteRepLog * m_pWriteRepLog;

        TMdbProcCtrl m_tProcCtrl;
    };

//}

#endif //__ZX_MINI_DATABASE_REP_CLIENT_CTRL_T_H__
