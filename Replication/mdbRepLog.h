/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepLog.h		
*@Description: 将共享内存区的同步数据按路由和主机落地到不同的文件，同一个路由对应多个备机时，会落地多次
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_LOG_H__
#define __ZTE_MINI_DATABASE_REP_LOG_H__

#include "Control/mdbRepCommon.h"
#include "Control/mdbProcCtrl.h"
#include "Helper/mdbQueue.h"
#include "Helper/mdbRepRecd.h"
#include "Control/mdbStorageEngine.h"
#include <string>

//namespace QuickMDB
//{
    /**
    * @brief 同步文件信息结构体
    * 
    */	
    class TRoutingRepFile: public TMdbNtcBaseObject
    {
    public:
        TRoutingRepFile();
        ~TRoutingRepFile();
    public:
        int m_iHostID;
        FILE *m_fp;
        std::string m_strFileName;
        time_t m_tCreateTime;
        int m_iBufLen;
        int m_iFilePos;
        char m_sFileBuf[MDB_MAX_REP_FILE_BUF_LEN];
    };
    /**
    * @brief 同步数据写文件类
    * 
    */
    class TRoutingRepLog
    {
    public:
        TRoutingRepLog();
        ~TRoutingRepLog();
    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	: 初始化，连接共享内存，读取配置文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(const char* sDsn, TMdbOnlineRepQueue & mdbOnlineRepQueueCtrl, int iHostID);

        int Log(bool bEmpty = false);	

    private:
        /******************************************************************************
        * 函数名称	:  Write
        * 函数描述	:  将指定路由数据写入对应的文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Write(int iRoutingID, const char* sDataBuf,  int iBufLen);
        /******************************************************************************
        * 函数名称	:  CheckAndBackup
        * 函数描述	: 初始化，连接共享内存，读取配置文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/      
        int CheckAndBackup();     

        void WriteToFile(TRoutingRepFile* pRepFile);

		/******************************************************************************
		* 函数名称	:  CreateRepFile
		* 函数描述	: 创建同步文件
		* 输入		:  
		* 输出		:  
		* 返回值	:  0 - 成功!0 -失败
		* 作者		:  jiang.lili
		*******************************************************************************/
		int CreateRepFile(TRoutingRepFile *ptRepFile, int iHostID);

        /******************************************************************************
        * 函数名称	:  Backup
        * 函数描述	: 备份同步文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Backup(TRoutingRepFile *pRepFile);
                
        /******************************************************************************
        * 函数名称	:  CheckWriteToFile
        * 函数描述	:  数据缓冲写文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int CheckWriteToFile(TRoutingRepFile*ptRepFile, const char* sDataBuf, int iBufLen);//缓冲写文件

    private:
        TMdbRepConfig *m_pRepConfig;//配置文件
        TMdbShmRepMgr *m_pShmMgr;//共享内存管理类
        const TMdbShmRep *m_pShmRep;//共享内存

        TMdbProcCtrl m_tProcCtrl;
        TMdbOnlineRepQueue *m_pOnlineRepQueueCtrl;//内存缓冲管理器
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn; 
        TMdbRepRecdDecode* m_pRecdParser;
        TMdbRedoLogParser m_tParser;
        int m_iMaxFileSize;//文件最大大小
        char m_sLogPath[MAX_PATH_NAME_LEN];
        long m_iRecord;//已处理的记录条数计数，超过10000条以后，从零开始重新计数
        int  m_iCheckCounts;
        int m_iLen;//存储一条记录长度
        char* m_spzRecord;//存储一条记录
        int m_iRoutID;//记录对应的路由ID
        int m_iHostID;
		TRoutingRepFile* m_pRoutingRepFile;
    };

	class TRoutingRepLogDispatcher
    {
    public:
        TRoutingRepLogDispatcher();
        ~TRoutingRepLogDispatcher();
    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	: 初始化，连接共享内存，读取配置文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
		int Init(const char* sDsn, TMdbQueue & mdbQueueCtrl, TMdbOnlineRepQueue * mdbOnlineRepQueueCtrl);
        int Dispatch(bool bEmpty = false);	

    private:
        TMdbRepConfig *m_pRepConfig;//配置文件
        TMdbShmRepMgr *m_pShmMgr;//共享内存管理类
        const TMdbShmRep *m_pShmRep;//共享内存
        TMdbQueue *m_pQueueCtrl;//内存缓冲管理器
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn; 
        TMdbRepRecdDecode* m_pRecdParser;
        TMdbRedoLogParser m_tParser;
        long m_iRecord;//已处理的记录条数计数，超过10000条以后，从零开始重新计数
        int m_iLen;//存储一条记录长度
        char* m_spzRecord;//存储一条记录
        int m_iRoutID;//记录对应的路由ID

		TMdbOnlineRepQueue * m_pOnlineRepQueueCtrl[MAX_ALL_REP_HOST_COUNT];
    };

//}

#endif
