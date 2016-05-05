/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepServerCtrl.h
*@Description： 负责同步数据接收，和数据加载时同步数据的发送
*@Author:		jiang.lili
*@Date：	    2014/03/20
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_REP_SERVER_CTRL_H__
#define __ZX_MINI_DATABASE_REP_SERVER_CTRL_H__


#include "Interface/mdbQuery.h"
#include "Replication/mdbRepNTC.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepFlushDao.h"
#include "Control/mdbProcCtrl.h"

//namespace QuickMDB
//{
    /**
    * @brief 接收备机同步数据类
    * 
    */
    class TRepServerDataRcv:public TMdbNtcBaseObject
    {
    public:
        TRepServerDataRcv();
        ~TRepServerDataRcv();
    public:
        int Init(const char* sDsn, const char* strIP, int iPort);//初始化
        int DealRcvData(const char* sDataBuf, int iBufLen);//处理接收到的数据
        bool IsSame(const char* strIP, int iPort);//是否是相同的备机
    private:
        int CombineData(const char* sDataBuf, int iBufLen);
        int GetRecdLen();
        int CheckDataCompletion();
        void SaveRecord();
       
    private:
        /*QuickMDB::*/TMdbNtcString m_strIP;//备机IP
        int m_iPort;//备机端口号
        char m_sCurTime[MAX_TIME_LEN]; // 当前时间
        char m_sLinkTime[MAX_TIME_LEN]; //  建链时间
        bool m_bCheckTime; // 是否校验时间戳       
        int m_iMsgBufLen;
        char* m_psMsgBuf;
        char m_sLeftBuf[MAX_REP_SEND_BUF_LEN];//剩余buf
        char m_sOneRecord[MAX_VALUE_LEN];//一条记录

        int m_iDealDataLen;//已经处理的Msg长度
        int m_iLeftDataLen;//剩余的Msg长度
        int m_iTotalDataLen;//数据的总长度

        TRepFlushDAOCtrl * m_ptFlushDaoCtrl;
    };

    /**
    * @brief 向备机发送启动时的加载数据
    * 
    */
    class TRepServerDataSend
    {
    public:
        TRepServerDataSend();
        ~TRepServerDataSend();
    public:
        int Init(const char* sDsn);
        int SendData(int iHostID, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo);//发送数据至备机
        int SendData(const char* sTableName, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo);//发送数据至备机
        int CleanRepFile(int iHostID=MDB_REP_EMPTY_HOST_ID);//删除对应主机的同步文件
    private:
        int DealOneTable(const char* sTableName, const char* sRoutinglist);
        int SendBufData(const char* sBuf, int iLength);//发送缓冲区中的数据
        void GetOneRecord();//处理一条记录
        
    private:
        TMdbDatabase *m_pDataBase;
        TMdbQuery *m_pCurQuery;//当前操作表指针
        TMdbConfig* m_pConfig;
        TMdbRepConfig *m_pRepConfig;
        
       TRepLoadDao *m_pLoadDao;//捞取数据
       //std::string m_strRoutingList;//需要加载的路由列表
       //int m_iHostID;//对应备机的ID
       TMdbPeerInfo* m_pPeerInfo;
       char m_sSendBuf[MAX_REP_SEND_BUF_LEN];//发送缓冲区
       char m_sOneRecord[MAX_VALUE_LEN];//一条记录
    };

    /**
    * @brief mdbRepServer流程管理类
    * 
    */
    class TRepServer
    {
    public:
        TRepServer();
        ~TRepServer();
        
    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(const char *pszDSN); //初始化
        int Start();//启动
    private:
        TMdbRepConfig *m_pRepConfig;//配置文件
        TMdbRepDataServer *m_pRepDataServer;//监听服务端
        TMdbShmRepMgr *m_pShmMgr;//共享内存管理类
        const TMdbShmRep *m_pShmRep;//共享内存

        TMdbProcCtrl m_tProcCtrl;
        
    };
//}


#endif //__ZX_MINI_DATABASE_SOCKET_REPLICATION_SERVER_H__

