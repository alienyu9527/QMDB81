/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepCommon.h		
*@Description: 定义分片备份所用的结构体和变量
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_COMMON_H__
#define __ZTE_MINI_DATABASE_REP_COMMON_H__

//#include "BillingSDK.h"
//#include "BillingNTC.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"
#include "Helper/TThreadLog.h"
#include "Helper/parser.h"
#include "Helper/mdbErr.h"
#include "Helper/mdbConfig.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>


//namespace QuickMDB
//{
#define ENV_QMDB_HOME_NAME "QuickMDB_HOME"
#define MAX_REP_SEND_MSG_LEN 32
#define MAX_REP_SEND_BUF_LEN 1024*32
#define MAX_REP_LINK_BUF_RESERVE_LEN 10*MAX_REP_SEND_BUF_LEN


#define IS_DISASTER_RECOVERY "1" //QuickMDB存在容灾机， 与配置服务通信标识
#define NOT_DISASTER_RECOVERY "0" //QuickMDB不存在容灾机，与配置服务通信标识
#define REP_ROUTING_ADD "A" //增加路由操作，与配置服务通信标识
#define  REP_ROUTING_DELETE "D" //删除路由操作，与配置服务通信标识

#define  MAX_REP_ROUTING_LIST_LEN 1024 //路由ID链表最大长度
#define  MAX_REP_HOST_COUNT 10 //一个路由最多的备份主机个数
#define MAX_REP_ROUTING_ID_COUTN 1000//一台主机上路由ID的最大个数
#define  MAX_ALL_REP_HOST_COUNT 100 //一台主机对应的最大备机数量
#define  MDB_SHM_ROUTING_REP_NAME "routing_rep" //路由映射关系共享内存名称
#define  MDB_REP_FILE_PATTEN "Rep.*.OK" //同步文件格式
#define  MDB_REP_EMPTY_HOST_ID -1 //未赋值的HostID
#define  MDB_MAX_REP_SERVER_THREAD 10//server端开启的最大接收数据线程数
#define  MDB_MAX_REP_FILE_LEN 1024*1024 //同步文件的最大长度
#define  MDB_MAX_REP_FILE_BUF_LEN 32*1024 //同步文件写缓冲区的最大长度

#define MAX_ONLINE_REP_BUF_USE_PERCENT 50 //飞行同步模式下最大的链路缓存占用率

const char MDB_REP_RCD_BEGIN[]= "^^";//同步记录的开始
const char MDB_REP_RCD_END[] = "##"; //同步记录的结束

#define LOAD_DATA_END_FLAG "Load-From-Peer-End"
#define LOAD_DATA_START_FLAG "Load:"
#define CLEAN_REP_FILE_FLAG "CleanRepFile"
#define REP_HEART_BEAT  "Heart"
#define REP_TABLE_NO_EXIST "Table-No-Exist"
#define HOST_ENDIAN_MATCH "Endian-Match"
#define HOST_ENDIAN_NOT_MATCH "Endian-Not-Match"
#define REP_TABLE_ERROR "Table-Rep-Failed"

/*#define REP_TABLE_DIFF_NULL_SET "Table-Diff-NULL-Set"
#define REP_TABLE_COLUMN_LEN_SHORT "Table-Column-Len-Short"
#define REP_TABLE_DIFF_COLUMN_ORDER "Table-Diff-Column-Order"
#define REP_TABLE_STRUCT_DIFF_TOO_MANY "Table-Struct-Diff-Too-Many"
#define REP_TABLE_NO_DEFAULT_VALUE "Table-No-Default-Value"
#define REP_TABLE_DIFF_COLUMN_TYPE "Table-Diff-Column-Type"
*/

    /**
	 * @brief 数据库状态
	 * 
	 */
    enum EMdbRepState
    {
        E_MDB_STATE_FIRST,
        E_MDB_STATE_UNKNOWN,
        E_MDB_STATE_CREATING,
        E_MDB_STATE_CREATED,
        E_MDB_STATE_RUNNING,
        E_MDB_STATE_DESTROYED,
        E_MDB_STATE_ABNORMAL,
        E_MDB_STATE_LAST
    };

     /**
	 * @brief 被监控的QuickMDB的状态
	 * 
	 */
    class TMdbRepState:public TMdbNtcBaseObject
    {
    public:
        int m_iHostID;//主机标识
        EMdbRepState m_eState; //当前状态
       // int m_iPno; //PID     
    };

    /**
	 * @brief 备机信息
	 * 
	 */
    struct TMdbShmRepHost
    {
        TMdbShmRepHost()
        {
            Clear();
        }
        void Clear()
        {
            m_iHostID = MDB_REP_EMPTY_HOST_ID;
            m_iPort = -1;
            memset(m_sIP, 0, MAX_IP_LEN);
			m_bSameEndian = false;
        }
        int m_iHostID; //主机标识
        int m_iPort; //端口号
        char m_sIP[MAX_IP_LEN]; //IP
        bool m_bSameEndian;  //备机与本机字节序是否一致
    };

     /**
	 * @brief 路由与备机的对应关系
	 * 
	 */
    struct TMdbShmRepRouting
    {
    public:
        TMdbShmRepRouting()
        {
            Clear();
        }
        void Clear()
        {
            m_iRoutingID = DEFALUT_ROUT_ID;
            for (int i = 0; i<MAX_REP_HOST_COUNT; i++)
            {
                m_aiHostID[i] = MDB_REP_EMPTY_HOST_ID;
            }
            m_iRecoveryHostID = MDB_REP_EMPTY_HOST_ID;

        }
        int m_iRoutingID;//路由ID
        int m_aiHostID[MAX_REP_HOST_COUNT];//对应的所有备机，按序号确定优先级
        int m_iRecoveryHostID; //容灾机
    };

    /**
	 * @brief 所有路由与备机映射关系管理区
	 * 
	 */
    struct TMdbShmRep
    {
        TMdbShmRep()
        {
            m_bNoRtMode = false;
            //m_bRunning = false;
            m_eState = E_MDB_STATE_UNKNOWN;
            m_iHostID = MDB_REP_EMPTY_HOST_ID;
            m_iHeartbeat = 0;
            m_iRoutingIDCount = 0;
            m_iRepHostCount = 0;
            m_iLocalPort = 0;
            memset(m_sRoutingList, 0, MAX_REP_ROUTING_LIST_LEN);
			for(int i = 0; i<MAX_ALL_REP_HOST_COUNT; i++)
			{
				m_bNetConnect[i] = true;
			}
        }
        void Clear()
        {
            m_bNoRtMode = false;
            //m_bRunning = false;
            m_eState = E_MDB_STATE_UNKNOWN;
            m_iHostID = MDB_REP_EMPTY_HOST_ID;
            m_iHeartbeat = 0;
            m_iRoutingIDCount = 0;
            m_iRepHostCount = 0;
            m_iLocalPort = 0;
            memset(m_sRoutingList, 0, MAX_REP_ROUTING_LIST_LEN);
            memset(m_sFailedRoutingList, 0, MAX_REP_ROUTING_LIST_LEN);
            for (int i = 0; i<MAX_ALL_REP_HOST_COUNT; i++)
            {
                m_arHosts[i].Clear();
				m_iRepMode[i] = -1;
				m_bNetConnect[i] = true;
            }
            for (int i = 0; i<MAX_REP_ROUTING_ID_COUTN; i++)
            {
                m_arRouting[i].Clear();
            }
        }

		const TMdbShmRepHost * GetRepHostByID(int iHostID) const
		{
			for(int i = 0; i<MAX_REP_ROUTING_LIST_LEN; i++)
			{
				if(m_arHosts[i].m_iHostID == iHostID)
				{
					return (const TMdbShmRepHost *)&m_arHosts[i];
				}
				if(i == MAX_REP_ROUTING_LIST_LEN - 1)
				{
					TADD_NORMAL("Can't find rep host [%d].", iHostID);
					return NULL;
				}
			}
			return NULL;
		}
		int SetRepMode(TMdbDSN * pDsn, int iHostID, int iRepMode)
		{
			TADD_FUNC("Start.");
			int iRet = 0;
			if(m_iRepMode[iHostID] == iRepMode)
			{
				return iRet;
			}
			else
			{
				m_iRepMode[iHostID] = iRepMode;
			}
			TADD_FUNC("Finish.");
			return iRet;
		}

		void SetNetState(int iHostID, bool bConnected)
		{
			m_bNetConnect[iHostID] = bConnected;
		}
		
        bool m_bNoRtMode; // 不区分路由的模式,仅用来支持旧版本的主备同步模式
        //bool m_bRunning;//mdbRep是否正在运行
        bool m_bRoutingChange;//是否发生了路由变更
        EMdbRepState m_eState;
        int m_iHostID; //本机对应的HostID
        int m_iHeartbeat;//心跳间隔
        int m_iRepHostCount;//备机的数量
        int m_iRoutingIDCount;//路由数量
        int m_iLocalPort;//本机的repServer的端口号
        char m_sRoutingList[MAX_REP_ROUTING_LIST_LEN];//本机的路由列表
        char m_sFailedRoutingList[MAX_REP_ROUTING_LIST_LEN];//加载失败的路由列表
        TMdbShmRepHost m_arHosts[MAX_ALL_REP_HOST_COUNT]; //本机对应的所有备机
        TMdbShmRepRouting m_arRouting[MAX_REP_ROUTING_ID_COUTN]; //本机的路由与其备机（包括容灾机）的对应关系
        bool m_bNetConnect[MAX_ALL_REP_HOST_COUNT];
		int m_iRepMode[MAX_ALL_REP_HOST_COUNT];
    };

     /**
	 * @brief 所有路由与备机映射关系共享内存管理类
	 * 
	 */
    class TMdbShmRepMgr
    {
    public:
        TMdbShmRepMgr(const char* sDsn);
        ~TMdbShmRepMgr();
    public:
        /******************************************************************************
        * 函数名称	:  Create
        * 函数描述	:  创建共享内存
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Create();
        /******************************************************************************
        * 函数名称	:  Attach
        * 函数描述	:  连接共享内存
        * 输入		:  
        * 输出		:  
        * 返回值	: 0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Attach();
        /******************************************************************************
        * 函数名称	:  Detach
        * 函数描述	:  断开与共享内存的连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Detach();
        /******************************************************************************
        * 函数名称	:  Destroy
        * 函数描述	:  销毁共享内存
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Destroy();

        /******************************************************************************
        * 函数名称	:  WriteRoutingInfo
        * 函数描述	:  将路由信息写入共享内存
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int WriteRoutingInfo(const char* pData, int iLen);

        /******************************************************************************
        * 函数名称	:  SetHostID
        * 函数描述	:  设置本机对应的HostID
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SetHostID(int iHostID)
        {
            CHECK_OBJ(m_pShmRep);
            m_pShmRep->m_iHostID = iHostID;
            return 0;
        }
        /******************************************************************************
        * 函数名称	:  SetHeartbeat
        * 函数描述	:  设置心跳间隔
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SetHeartbeat(int iHeartbeat)
        {
            CHECK_OBJ(m_pShmRep);
            m_pShmRep->m_iHeartbeat = iHeartbeat;
            return 0;
        }
        /******************************************************************************
        * 函数名称	:  SetMdbState
        * 函数描述	:  设置MDB状态
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SetMdbState(EMdbRepState eState)
        {
            CHECK_OBJ(m_pShmRep);
            m_pShmRep->m_eState = eState;
            return 0;
        }
        /******************************************************************************
        * 函数名称	:  SetRunningFlag
        * 函数描述	:  标识mdb是否在运行
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SetRunningFlag(bool bRunning)
        {
            CHECK_OBJ(m_pShmRep);
            //m_pShmRep->m_bRunning = bRunning;
            return 0;
        }

        /******************************************************************************
        * 函数名称	:  GetRoutingRep
        * 函数描述	:  获取共享内存管理区指针
        * 输入		:  
        * 输出		:  
        * 返回值	: 
        * 作者		:  jiang.lili
        *******************************************************************************/
        const TMdbShmRep *GetRoutingRep()
        {
            return m_pShmRep;
        }

       /******************************************************************************
        * 函数名称	:  GetRepHosts
        * 函数描述	:  根据路由ID，获取其对应的所有备机
        * 输入		:  
        * 输出		:  
        * 返回值	:  
        * 作者		:  jiang.lili
        *******************************************************************************/
        TMdbShmRepRouting* GetRepHosts(int iRoutingID);

       /******************************************************************************
        * 函数名称	:  AddFailedRoutingList
        * 函数描述	:  将加载失败的路由列表写入共享内存
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int AddFailedRoutingList(const char* sRoutingList);

         /******************************************************************************
        * 函数名称	:  AddFailedRoutingID
        * 函数描述	:  将加载失败的路由ID写入共享内存
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int AddFailedRoutingID(int iRoutingID);

       /******************************************************************************
        * 函数名称	:  GetFailedRoutingList
        * 函数描述	:  获取加载失败的路由ID列表
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        const char* GetFailedRoutingList();

    private:
        TMdbNtcString m_strName;
        TMdbShmRep* m_pShmRep;
        TMdbNtcShareMem* m_pShm;
    };

    class TMdbRoutingTools
    {
    public:
        static bool IsIDInString(int iID, const char* sIDList);//ID是否在ID列表中
        static bool IsIDInArray(int iID, std::vector<int>&tIDArray);//ID是否在数组中
        static int GetIntArrayFromStrList(const char* sIDList, std::vector<int> &tIDArray);//根据ID列表构建ID数组
        static int GetIDArrayFromRule(const char* sRule, std::vector<int>& tIDArray);//根据规则，产生RoutingID数组
    };

    /*
    struct TRepRecord
    {
        int m_iRcdLen;
        int m_iRoutingID;
        int m_iSqlType;
        char m_sTableName[MAX_NAME_LEN];
        char m_sData[];      
    };*/

    class TMdbRepConfig
    {
    public:
        TMdbRepConfig();
        ~TMdbRepConfig();

        int Init(const char* sDsn);
        
    public:
        std::string m_sServerIP;//配置服务IP
        int m_iServerPort;//配置服务端口号

        std::string m_sRepServerIP;//配置服务备机IP
        int m_iRepServerPort;//配置服务备机端口号

        std::string m_sRepPath;//同步文件的目录
        time_t m_iFileInvalidTime;//同步文件的过期时间
        int m_iMaxFileSize;//同步文件最大大小
        int m_iBackupInterval;//同步文件备份间隔

        std::string m_sLocalIP;//本地IP
        int m_iLocalPort;//DSN repServer 使用的端口号

    private:
        TMdbConfig  *m_pConfig;
    };

	class TMdbTableChangeOper
	{
	public:
		TMdbTableChangeOper();
		~TMdbTableChangeOper();

		void Clear();
	public:
		int m_iColPos;
		int m_iColType;//新增加列的类型
		int m_iColLen;
		int m_iChangeType;
		int m_iValue;
		long long m_llValue;
		char m_sValue[MAX_DEFAULT_VALUE_LEN];
	};

	class TMdbLoadTableRemoteStruct
	{
	public:
		TMdbLoadTableRemoteStruct();
		~TMdbLoadTableRemoteStruct();

		void Clear();
	public:
		int m_iIsLocalCol[MAX_COLUMN_COUNTS]; //0-本地列，1-对端列，-1-没有列
		int m_iColPos[MAX_COLUMN_COUNTS];//本地列的列号，对端列对应的change序列
		int m_iColCount;
	};
//}
#endif
