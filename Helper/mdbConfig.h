/****************************************************************************************
*@Copyrights  2007，中兴软创（南京）计算机有限公司 开发部 CCB项目组
*@                   All rights reserved.
*@Name：	    TMdbConfig.h
*@Description： minidb的配置文件控制类
*@Author:		li.shugang
*@Date：	    2008年11月25日
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_CONFIG_H__
#define __ZTE_MINI_DATABASE_CONFIG_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbXML.h"
#include "Helper/TMutex.h"
#include "Helper/TDBFactory.h"

#include <string>
#include <vector>
#ifndef WIN32
#include <unistd.h>  //access函数
#else
#include <stdio.h>   //access函数
#include <io.h>
#endif
#ifdef WIN32
#pragma warning(disable:4018)
#endif

#include "Helper/mdbDictionary.h"
//#include "BillingSDK.h"

#include "Helper/mdbConfigAlter.h"
//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{


#define SET_SYS_PARAM_INT_VALUE(PARAM,NAME,COMP,VALUE,DEFAUT)\
    if(TMdbNtcStrFunc::StrNoCaseCmp(NAME,COMP) == 0)\
    {\
        if (0 == TMdbNtcStrFunc::StrNoCaseCmp(VALUE,""))\
        {\
            PARAM = DEFAUT;\
        }\
        else\
        {\
            PARAM = atoi(VALUE);\
            if(PARAM < 0)\
          	{\
          		PARAM = DEFAUT;\
          	}\
        }\
        continue;\
    }

#define SET_SYS_PARAM_SIZE_VALUE(PARAM,NAME,COMP,VALUE,DEFAUT)\
    if(TMdbNtcStrFunc::StrNoCaseCmp(NAME,COMP) == 0)\
    {\
        if (0 == TMdbNtcStrFunc::StrNoCaseCmp(VALUE,""))\
        {\
            PARAM = DEFAUT*1024*1024;\
        }\
        else\
        {\
            PARAM = atol(VALUE)*1024*1024;\
            if(PARAM < 0)\
          	{\
          		PARAM = DEFAUT*1024*1024;\
          	}\
        }\
        continue;\
    }

#define SET_PARAM_BOOL_VALUE(PARAM,NAME,COMP,VALUE)\
    if(TMdbNtcStrFunc::StrNoCaseCmp(NAME,COMP) == 0)\
    {\
        if (0 == TMdbNtcStrFunc::StrNoCaseCmp(VALUE,""))\
        {\
            PARAM = false;\
        }\
        else\
        {\
            if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "Y") == 0)\
            {\
                PARAM = true;\
            }\
            else if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "N") == 0)\
            {\
                PARAM = false;\
            }\
            else\
            {\
                TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s=%s].",COMP,VALUE);\
                return ERR_APP_INVALID_PARAM;\
            }\
        }\
        continue;\
    }
#define SET_SYS_BOOL_VALUE(PARAM,NAME,COMP,VALUE,DEFAUT)\
if(TMdbNtcStrFunc::StrNoCaseCmp(NAME,COMP) == 0)\
{\
    if (0 == TMdbNtcStrFunc::StrNoCaseCmp(VALUE,""))\
    {\
        PARAM = DEFAUT;\
    }\
    else\
    {\
        if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "Y") == 0)\
        {\
            PARAM = true;\
        }\
        else if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "N") == 0)\
        {\
            PARAM = false;\
        }\
        else\
        {\
            TADD_ERROR("Invalid element=[%s=%s].",COMP,VALUE);\
            return ERR_APP_INVALID_PARAM;\
        }\
    }\
    continue;\
}

#define SET_SYS_PARAM_CHAR_VALUE(PARAM,LEN,NAME,COMP,VALUE)\
    if(TMdbNtcStrFunc::StrNoCaseCmp(NAME,COMP) == 0)\
    {\
        if (0 == TMdbNtcStrFunc::StrNoCaseCmp(VALUE,""))\
        {\
            memset(PARAM,0,LEN);\
        }\
        else\
        {\
            strncpy(PARAM,VALUE,LEN-1);\
        }\
        continue;\
    }

#define SET_TABLE_PROPERTY_BOOL_VALUE(PARAM,PROPERTY,VALUE)\
    if(NULL != VALUE)\
    {\
        if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "Y") == 0)\
        {\
            PARAM = true;\
        }\
        else if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "N") == 0)\
        {\
            PARAM = false;\
        }\
        else\
        {\
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s=%s].",PROPERTY,VALUE);\
            return ERR_APP_INVALID_PARAM;\
        }\
    }

    //mdb NULL值管理
    class TMdbNULLManager
    {
    public:
        static int CalcNullSize(TMdbTable * pTable);//获取该表需要的NULLsize
        static bool IsColumnNULL(char * pAddr,int iColumnPos);//判断该列是否为NULL
    };
    
    class TMDbUser
    {
    public:
        void Clear()
        {
            memset(sUser,   0, sizeof(sUser));
            memset(sPwd,    0, sizeof(sPwd));
            memset(sAccess, 0, sizeof(sAccess));
        }

        char sUser[MAX_NAME_LEN];
        char sPwd[MAX_NAME_LEN];
        char sAccess[MAX_NAME_LEN];
    };

    //配置文件的结构表示--QuickMDB_SYS_XXX.xml
    class TMdbCfgDSN
    {
    public:
        TMdbCfgDSN();
        ~TMdbCfgDSN();

    	void Init();
        void Clear();
        void Print();

    public:
        char sName[MAX_NAME_LEN];
        //int  iValue;              //DSN对应的值，考虑到随机的数据可能有冲突
        long long llValue; //根据DSN 与QuickMDB_HOME 算出的hash值，替换iValue
        int iLogBuffSize;
        char sLogDir[MAX_PATH_NAME_LEN];
        int iLogFileSize;

        int iRepBuffSize;
        char sRepDir[MAX_PATH_NAME_LEN];
        int iRepFileSize;

        int iCaptureBuffSize;
        char sCaptureDir[MAX_PATH_NAME_LEN];
        int iCaptureFileSize;

        int iRedoBuffSize;
        char sRedoDir[MAX_PATH_NAME_LEN];
        int iRedoFileSize;

        char sDataStore[MAX_PATH_NAME_LEN];
        int  iPermSize;
        char sStorageDir[MAX_PATH_NAME_LEN];//文件存储位置

        char sOracleID[MAX_NAME_LEN];
        char sOracleUID[MAX_NAME_LEN];
        char sOraclePWD[MAX_NAME_LEN];

        char sLocalIP[MAX_IP_LEN];   //本地IP
        char sLocalIP_active[MAX_IP_LEN];   //本地IP,兼容1.2的local_active_ip
        char sPeerIP[MAX_IP_LEN];    //对端IP
        char sActiveIP[MAX_IP_LEN];  //容灾活动IP
        char sStandByIP[MAX_IP_LEN]; //容灾standbyIP
        int  iLocalPort;             //本地端口
        int  iPeerPort;              //对端端口
        int  iActivePort;            //容灾活动端口
        int  iStandbyPort;             //容灾standby端口
		char sAgentPortStr[64]; 	 //代理端口列表字符串
		int  iAgentPort[MAX_AGENT_PORT_COUNTS]; 			//代理端口列表
		
		char sNtcPortStr[64];		 //使用ntc的端口列表字符串
		char sNoNtcPortStr[64];      //不使用ntc端口列表字符串
		int  iNtcPort[MAX_AGENT_PORT_COUNTS];               //使用ntc的端口列表
		int  iNoNtcPort[MAX_AGENT_PORT_COUNTS];             //不使用ntc的端口列表
		
        int  iLogLevel;              //日志级别
        bool bIsReadOnlyAttr;        //DSN的访问属性
        bool bIsOraRep;              //是否开启Oracle的同步

		//begin 兼容1.2
        bool bIsPeerRep;            //是否开启容灾备份
        bool bIsRep;                 //是否开启主备同步
        //end
        
        bool bIsCaptureRouter;            //是否开启路由捕获
        bool bIsLicense;             //是否需要检测序列号

        int iKillTime;              //杀死进程的时间，以秒为单位
        int iRepType;               //1-异步(速度最快); 2-半同步(速度降低50%,但是更加安全); 3-全同步(速度最慢，但是安全性最高)
        int iOraRepCounts;          //Oracle同步进程的个数
        int iNetDropTime;           //允许的断网时间，以秒为单位，如果实际时间小于X秒(闪断的概念)，则认为可以继续传送数据，否则全量同步数据
        int iDelayTime;         //数据从Oracle到内存数据库的延迟时间,单位为秒
        int iCleanTime;         //清理%DSN%_MDB_CHANGE_NOTIF 表数据的时间,单位min
        int iAgentThreadStackSize;
        int iLoadThreadStackSize;

        char cType;
        char checkDataByOra[MAX_PATH_NAME_LEN];
        char checkDataByMdb[MAX_PATH_NAME_LEN];
        char errorLog[MAX_PATH_NAME_LEN];
        int iLogTime;
		int iLogCount;
        size_t iManagerSize;
        size_t iDataSize;
        bool bIsCheckPriKey;
        int iClientTimeout;//CS模式的客户端提供超时机制，单位秒
        char sRoutingName[MAX_NAME_LEN]; //路由ID列名称
        char sRoutingList[MAX_ROUTER_LIST_LEN];//DSN中数据对应的路由列表[ur: 415074 新增]
        int m_iOraRepInterval; // ORACLE2MDB刷新时间间隔，单位秒
        int m_iOraRepDelaySec; // ORACLE2MDB 漏刷时间， 单位秒
        int m_iSeqCacheSize;  //序列在create时cache的大小
        bool m_bIsDiskStorage;//是否开启磁盘存储
        int m_iLongSqlTime;//长时间的sql
        int m_iRepFileTimeout;//主备同步文件超时，默认24小时，单位 : 小时
        int m_iCSPumpInitCount;//CS模式使用事件泵初始个数
        int m_iCSPumpMaxCount;//事件泵上限
        int m_iCSPeerCountPerPump;//每个事件泵处理的peer个数
		char  m_sCSValidIP[MAX_IP_LEN*16+1];//有效cs ip
		char  m_sCSInValidIP[MAX_IP_LEN*16+1];//无效cs ip
        
        bool m_bShadow; // ORACLE2MDB同步是否开启影子表模式
        bool m_bSingleDisaster;  //是否单机容灾部署
        bool m_bNull;//设置null处理规则:true 表示与nil四则运算还为nil;false 表示nil运算时当0值处理(兼容老版本)。        
        bool m_bIsShardBackup;		 //是否开启分片备份
        bool m_bUseNTC;//CS是否使用NTC

        // 分片备份相关配置项
        char sRepSvrIp[MAX_IP_LEN];
        char sRepStandbySvrIp[MAX_IP_LEN];
        int iRepSvrPort;
        int iRepStandbySvrPort;
        //char sRepLocalIp[MAX_IP_LEN];
        bool m_bSQLIsAnswer;//mdbSQL 执行重大操作，是否需要交互,默认交互
        int iRepLocalPort;
        int iInvalidRepFileTime;
        char sRepFilePath[MAX_PATH_NAME_LEN];
        std::vector<std::string> m_vNotLoadFromDBTab;//不需要从数据库加载的表
        std::string m_strNotLoadFromDBList;//未分隔的表列表
        // 加载优先级
        int m_iLoadPriority; // 1 - file storage first; 2- db storage first;

		bool m_bReloadOra;//是否进行oracle 二次登陆校验
    	bool m_bReloadEncrypt;//二次登陆是否加密
    	char m_sReloadCfgName[MAX_PATH_NAME_LEN];//二次登陆配置文件
    	char m_sReloadDbType[MAX_NAME_LEN];//二次登陆数据库类型
    };

    class TMdbCfgProAttr
    {
    public:
        TMdbCfgProAttr()
        {
            bAutoRestart = true;
            iHeartBeatWarning = 10;
            iHeartBeatFatal   = 30;
        }
        bool bAutoRestart;
        int iHeartBeatWarning;
        int iHeartBeatFatal;
    };

    class TMdbConfig
    {
    public:
        TMdbConfig();
        ~TMdbConfig();

    public:
        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  初始化配置文件
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Init();

		int ParseAgentPort();
		int ParseNtcAgentPort(char * sPortStr,int * iPortArray);

        /******************************************************************************
        * 函数名称	:  SetFlag()
        * 函数描述	:  设置动作标识，是否需要检测记录数。仅供mdbCtrl调用
        * 输入		:  bFlag， 如果需要检测记录数,则为true,否则为false
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetFlag(bool bFlag);

        /******************************************************************************
        * 函数名称	:  LoadCfg()
        * 函数描述	:  装载某个DSN的配置
        * 输入		:  pszDsn, DSN名称
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadCfg(const char* pszDsn);

        /******************************************************************************
        * 函数名称	:  LoadSysCfg()
        * 函数描述	:  装载系统配置
        * 输入		:  cSysCfgFile, 系统配置文件
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadSysCfg(const char* cSysCfgFile,const bool bCheck=true);

        /******************************************************************************
        * 函数名称	:  LoadTabCfg()
        * 函数描述	:  装载用户表配置
        * 输入		:  cTabCfgFile, 用户表名
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTabStruct(const char* cTabCfgFile);

        /******************************************************************************
        * 函数名称	:  LoadTabCfgEx()
        * 函数描述	:  装载用户表配置
        * 输入		:  cTabCfgFile, 用户表名
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTabProperty(const char* cTabCfgFile);

        /******************************************************************************
        * 函数名称	:  CheckParam()
        * 函数描述	:  检查用户名和密码的正确性
        * 输入		:  pszUser, 用户名
        * 输入		:  pszPWD, 密码
        * 输入		:  pszDsn, DSN名称
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int CheckParam(const char* pszUser, const char* pszPWD, const char* pszDsn);

        /******************************************************************************
        * 函数名称	:  GetTable()
        * 函数描述	:  获取某个表信息
        * 输入		:  pszTable, 表名称
        * 输出		:  无
        * 返回值	:  成功返回表结构指针，否则返回NULL
        * 作者		:  li.shugang
        *******************************************************************************/
        TMdbTable* GetTable(const char* pszTable);

        // 获取旧的表结构配置信息
        TMdbTable* GetOldTableStruct(const char* pszTabName);

        /******************************************************************************
        * 函数名称	:  GetTableCounts()
        * 函数描述	:  获取表数目
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回表数目
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetTableCounts();

        /******************************************************************************
        * 函数名称	:  GetTableSpaceCounts()
        * 函数描述	:  获取表空间数目
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回表空间数目
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetTableSpaceCounts();

        /******************************************************************************
        * 函数名称	:  GetUserCounts()
        * 函数描述	:  获取用户个数
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回用户数目
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetUserCounts();

        /******************************************************************************
        * 函数名称	:  SetUserCounts()
        * 函数描述	:  设置用户个数
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetUserCounts(int iCount);

    	void SetTablespaceCounts(int iCount);
    	void SetTableCounts(int iCount);
        /******************************************************************************
        * 函数名称	:  GetTableByPos()
        * 函数描述	:  根据位置获取表信息
        * 输入		:  iPos，表的位置
        * 输出		:  无
        * 返回值	:  返回表信息指针
        * 作者		:  li.shugang
        *******************************************************************************/
        TMdbTable* GetTableByPos(int iPos);
        TMdbTable* GetTableByTableId(int iTableId);

    	TMdbTable* GetTableByIdxName(const char* pszIdxName);

        /******************************************************************************
        * 函数名称	:  GetTableSpace()
        * 函数描述	:  获取表空间信息
        * 输入		:  iPos，表空间的位置
        * 输出		:  无
        * 返回值	:  返回表空间信息指针
        * 作者		:  li.shugang
        *******************************************************************************/
        TMdbTableSpace* GetTableSpace(int iPos);

    	TMdbTableSpace* GetTableSpaceByName(const char* pszTablespaceName);
    	TMdbTableSpace* GetIdleTableSpace();
    	TMdbTable* GetIdleTable();

        /******************************************************************************
        * 函数名称	:  GetSeqCounts()
        * 函数描述	:  获取序列的数目
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回序列的数目
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetSeqCounts();

        /******************************************************************************
        * 函数名称	:  GetSequence()
        * 函数描述	:  获取序列的信息
        * 输入		:  iPos，序列的位置
        * 输出		:  无
        * 返回值	:  返回序列信息指针
        * 作者		:  li.shugang
        *******************************************************************************/
        TMemSeq* GetSequence(int iPos);

        /******************************************************************************
        * 函数名称	:  GetDSN()
        * 函数描述	:  获取DSN信息
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回DSN信息指针
        * 作者		:  li.shugang
        *******************************************************************************/
        TMdbCfgDSN* GetDSN();

        /******************************************************************************
        * 函数名称	:  GetProAttr()
        * 函数描述	:  获取TMdbCfgProAttr信息
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回TMdbCfgProAttr信息指针
        * 作者		:  li.shugang
        *******************************************************************************/
        TMdbCfgProAttr* GetProAttr();

        /******************************************************************************
        * 函数名称	:  IsWriteUnLock()
        * 函数描述	:  是否有类似OCS_SESSION一类的频繁插入/删除的表
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  有则返回true，否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        bool IsWriteUnLock();

        /******************************************************************************
        * 函数名称	:  GetAccess()
        * 函数描述	:  获取链接的操作权限
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回链接的操作权限
        * 作者		:  li.shugang
        *******************************************************************************/
        char GetAccess()
        {
            return m_cAccess;
        }

        /******************************************************************************
        * 函数名称	:  GetUser()
        * 函数描述	:  获取用户列表
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  返回链接的操作权限
        * 作者		:  li.shugang
        *******************************************************************************/
        TMDbUser* GetUser(int iPos)
        {
            return m_pUser[iPos];
        }

    	TMDbUser* GetUser(const char* pszUserName);
        /******************************************************************************
        * 函数名称	:  CheckLicense()
        * 函数描述	:  检测序列号是否正确
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  正确则返回true，否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        bool CheckLicense();

        /******************************************************************************
        * 函数名称	:  IsAdmin()
        * 函数描述	:  判断当前用户是否是管理员权限
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  是返回true，否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        bool IsAdmin();

        /******************************************************************************
        * 函数名称	:  GetDataType()
        * 函数描述	:  根据字段类型获取内部类型
        * 输入		:  pszDataTpye 字段类型
        * 输出		:  无
        * 返回值	:  内部数据类型
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetDataType(const char* pszDataTpye);

        /******************************************************************************
        * 函数名称	:  GetRepType()
        * 函数描述	:  获取同步类型
        * 输入		:  pszRepTpye 配置的同步类型
        * 输出		:  无
        * 返回值	:  转换后内部同步类型
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetRepType(const char* pszRepTpye);

    	/******************************************************************************
    	* 函数名称	:  GetDataType
    	* 函数描述	:  获取数据类型的字符串表示
    	* 输入		:
    	* 输入		:
    	* 输出		:
    	* 返回值	:  0 - 成功!0 -失败
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	int GetDataType(int iDataType,char *pDataType,const int iLen);
    	
    	/******************************************************************************
    	* 函数名称	:  GetRepType
    	* 函数描述	:  获取数据同步类型的字符串表示
    	* 输入		:
    	* 输入		:
    	* 输出		:
    	* 返回值	:  0 - 成功!0 -失败
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	int GetRepType(int iRepType,char *pRepType,const int iLen);
    	
        /******************************************************************************
        * 函数名称	:  GetConfigFileName()
        * 函数描述	:  获取系统配置文件名
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  系统配置文件名
        * 作者		:  li.shugang
        *******************************************************************************/
        char * GetConfigFileName();

        /******************************************************************************
        * 函数名称	:  GetIsStartFlushFromOra()
        * 函数描述	:  是否启动从oracle同步数据进程
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  是返回true，否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        bool GetIsStartFlushFromOra();

        /******************************************************************************
        * 函数名称	:  GetIsStartOracleRep()
        * 函数描述	:  获取是否向oracle同步数据进程
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  是返回true，否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        bool GetIsStartOracleRep();

        bool GetIsStartShardBackupRep();

        bool GetIsStartFileStorageProc();

        /******************************************************************************
        * 函数名称	:  GetConfigHomePath()
        * 函数描述	:  获取配置文件 homepath
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  配置文件路径
        * 作者		:  li.shugang
        *******************************************************************************/
        char * GetConfigHomePath();
        int GetDbDataShmCounts(int &iDataShmCounts);//获取数据库需要的数据共享块
        int GetOneTableDataSize(TMdbTable * pTable,long long &llDataSize);//获取某张表所需要的数据空间大小
        int GetOneTableSpaceDataSize(char* psTableSpaceName,long long &llDataSize);//获取某个表空间所需要的数据空间大小
        bool IsDiffToMemTable(TMdbTable * pCfgTable,TMdbTable * pMemTable);//是否和内存表一致
        
        /******************************************************************************
        * 函数名称	:  CalcOneRecordSize
        * 函数描述	:  计算表中一条记录的大小
        * 输入		:  pTable - 需要计算的表
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  jin.shaohua
        *******************************************************************************/
        int CalcOneRecordSize(TMdbTable * pTable);

    	/******************************************************************************
        * 函数名称	:  SetColOffset()
        * 函数描述	:  填充列的偏移量信息
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  设置成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int SetColOffset(TMdbTable* pTable);
    	
    	/******************************************************************************
        * 函数名称	:  IsExist()
        * 函数描述	:  检查配置文件是否存在
        * 输入		:  pszDsn DSN名称
        * 输出		:  无
        * 返回值	:  存在返回true,否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        bool IsExist(const char* pszDsn);

        /******************************************************************************
        * 函数名称	:  HasWhereCond()
        * 函数描述	:  判断一张表的Load SQL语句中是否包含where条件
        * 输入		:  sTableName 表名
        * 输入		:  sLoadSQL sql语句
        * 输出		:  无
        * 返回值	:  存在返回true,否则返回false
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool HasWhereCond(const char* sTableName, const char * sLoadSQL);
    	/******************************************************************************
    	* 函数名称	:  IsValidTableOrColumnName
    	* 函数描述	:  判断表名或列名是否合法
    	* 输入		:  pName表名或列名
    	* 输出		:  无
    	* 返回值	:  true 和法;false 非法
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	bool IsValidName(const char *pName);
    	
    	/******************************************************************************
    	* 函数名称	:  CheckColumnProperty()
    	* 函数描述	:  校验指定列的长度、同步属性等
    	* 输入		:  pTable 检验表名，iColumnPos 检验列位置，
    	* 输出		:  无
    	* 返回值	:  0:成功；!0 : 失败
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	int CheckColumnProperty(TMdbTable* pTable, const int iColumnPos);
    	/******************************************************************************
    	* 函数名称  :  CheckPKIsIndexed()
    	* 函数描述  :  检查主键列是否建索引
    	* 输入		:  pTable 表对象
    	* 输出		:  无
    	* 返回值    :  成功返回0，否则返回-1
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	int CheckPKIsIndexed(TMdbTable * pTable);		
		int CheckColumnCfg(TMdbTable * pTable);
    	int LoadOnSiteFile(const char* pszDsn);//加载onsite文件
    	TMdbTable* GetTableByName(const char* pszTableName);//通过表名获取表信息
    	int LoadDsnConfigFile(const char* pszDsn,const bool bCheck);
        static long long GetDsnHashValue(const char *sDsn);//根据dsn+QuickMDB_HOME 计算hash值

        int BackUpConfig();
        int ClearAlterFile();
        
    private:
        /******************************************************************************
        * 函数名称	:  LoadDsnCfg()
        * 函数描述	:  读取XML中的数据源信息
        * 输入		:  pRoot XML文件根节点
        * 输出		:  无
        * 返回值	:  成功加载返回true，否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        bool LoadDsnCfg(MDBXMLElement* pRoot);

        /******************************************************************************
        * 函数名称	:  LoadDsnInfo()
        * 函数描述	:  读取数据源信息
        * 输入		:  pEle DataSource节点
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDsnInfo(MDBXMLElement* pEle,const bool bCheck);

        /******************************************************************************
        * 函数名称	:  GetConfigHomePath()
        * 函数描述	:  读取sys节点数据源信息
        * 输入		:  pEle sys节点
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadNtcPortsInfo(MDBXMLAttribute* pAttr,MDBXMLAttribute* pAttrValue);
        int LoadSysInfoFromXML(MDBXMLElement* pESys);
        int LoadSysInfo(MDBXMLElement* pEle,const bool bCheck);

        /******************************************************************************
        * 函数名称	:  LoadTableCfg()
        * 函数描述	:  读取 表 配置
        * 输入		:  pMDB table节点
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTableStruct(const char* psTableDir, const char* psTableName);

        /******************************************************************************
        * 函数名称	:  LoadTableSpaceCfg()
        * 函数描述	:  读取 表空间 配置
        * 输入		:  pMDB table-space节点
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTableSpaceCfg(MDBXMLElement* pMDB,const bool bCheck);

        /******************************************************************************
        * 函数名称	:  LoadUser()
        * 函数描述	:  读取 用户 配置
        * 输入		:  pMDB user节点
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadUser(MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称	:  CheckCfg()
        * 函数描述	:  检测配置
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  0表示配置ok,非0表示配置非法
        * 作者		:  li.shugang
        *******************************************************************************/
        int CheckCfg();

        /******************************************************************************
        * 函数名称	:  LoadTableColumn()
        * 函数描述	:  上载表相关的列信息
        * 输入		:  pMDB table节点
        * 输出		:  pTable 表信息，bIsInOra 是否与oracle操作
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTableColumn(TMdbTable* pTable, MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称	:  LoadTableIndex()
        * 函数描述	:  上载表相关的索引信息
        * 输入		:  pMDB 表节点
        * 输出		:  pTable 表信息
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTableIndex(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称  :  LoadRelation()
        * 函数描述  :  上载子表和主表的对映关系
        * 输入	   :  pMDB
        * 输出	   :  pChildTable 子表信息
        * 返回值    :  加载成功返回0,否则返回-1
        * 作者	   :  li.shugang
        *******************************************************************************/
        //int LoadRelation(TMdbChildTable* pChildTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称	:  LoadTablePrimaryKey()
        * 函数描述	:  上载表相关的主键信息
        * 输入		:  pMDB pkey节点
        * 输出		:  pTable 表信息
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadTablePrimaryKey(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称	:  LoadSQL()
        * 函数描述	:  上载SQL 信息
        * 输入		:  pMDB table节点
        * 输出		:  pTable 表信息
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadSQL(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称	:  LoadChildInfo()
        * 函数描述	:  上载子表信息
        * 输入		:  pMDB table节点
        * 输出		:  pTable 表信息
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        //int LoadChildInfo(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称	:  LoadParameter()
        * 函数描述	:  上载参数配置
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadParameter(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * 函数名称	:  LoadSysTable()
        * 函数描述	:  填充系统表信息
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadSysTable();

        int LoadTable();

        

        /******************************************************************************
        * 函数名称	:  LoadDBATables()
        * 函数描述	:  添加dba_tables
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBATables();

        /******************************************************************************
        * 函数名称	:  LoadDBAColumn()
        * 函数描述	:  添加dba_column
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBAColumn();

        /******************************************************************************
        * 函数名称	:  LoadDBAIndex()
        * 函数描述	:  添加dba_index
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBAIndex();

        /******************************************************************************
        * 函数名称	:  LoadDBATableSpace()
        * 函数描述	:  添加dba_tablespace
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBATableSpace();

        /******************************************************************************
        * 函数名称	:  LoadDBASequence()
        * 函数描述	:  添加dba_sequence
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBASequence();

        /******************************************************************************
        * 函数名称	:  LoadDBAUser()
        * 函数描述	:  添加dba_user
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBAUser();

        /******************************************************************************
        * 函数名称	:  LoadDBASession()
        * 函数描述	:  添加dba_session
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBASession();

        /******************************************************************************
        * 函数名称	:  LoadDBAResource()
        * 函数描述	:  添加dba_resource
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBAResource();

        /******************************************************************************
        * 函数名称	:  LoadDBASQL()
        * 函数描述	:  添加dba_sql
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  加载成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int LoadDBASQL();

    	/******************************************************************************
    	* 函数名称	:  LoadDBAProcess()
    	* 函数描述	:  添加dba_process
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  加载成功返回0,否则返回-1
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	int LoadDBAProcess();

        int LoadDBADual();//加载dual表
        /******************************************************************************
        * 函数名称	:  SetColumn()
        * 函数描述	:  设置系统表的某一个列信息
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetColumn(TMdbTable* pTable, int iCPos, const char* pszColumnName, int iDataType, int iDataLen);

        /******************************************************************************
        * 函数名称	:  GetLincese()
        * 函数描述	:  获取配置文件 homepath
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void GetLincese(char* pLicense);

        /******************************************************************************
        * 函数名称	:  SetConfigHomePath()
        * 函数描述	:  设置配置文件home路径
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  设置成功返回0,否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int SetConfigHomePath(const char* pszDsn);
        int LoadMdbJob();//上周mdbjob数据
        /******************************************************************************
    	* 函数名称	:  AddRouteIDForLoadSQL()
    	* 函数描述	:  为表刷新SQL添加路由
    	* 输入		:  pTable 表
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	void AddRouteIDForLoadSQL(TMdbTable* pTable);

    	/******************************************************************************
    	* 函数名称	:  GetTablePropertyValue()
    	* 函数描述	:  读取 表 配置，获取指定属性的值
    	* 输入		:  pMDB table节点,pPropertyName 属性名
    	* 输出		:  无
    	* 返回值	:  属性值
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	char * GetTablePropertyValue(MDBXMLElement* pMDB,const char* pPropertyName);
    	/******************************************************************************
    	* 函数名称	:  PrimaryKeyCmp
    	* 函数描述	:  比较两个表的主键是否一致
    	* 输入		:  
    	* 输入		:  
    	* 输出		:  
    	* 返回值	:  true 一致;false 不一致
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	bool TablePropertyCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);
    	/******************************************************************************
    	* 函数名称	:  PrimaryKeyCmp
    	* 函数描述	:  比较两个表的主键是否一致
    	* 输入		:  
    	* 输入		:  
    	* 输出		:  
    	* 返回值	:  true 一致;false 不一致
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	bool PrimaryKeyCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);
    	/******************************************************************************
    	* 函数名称	:  ColumnAttrCmp
    	* 函数描述	:  比较两个表的列是否一样
    	* 输入		:  
    	* 输入		:  
    	* 输出		:  
    	* 返回值	:  true 一致;false 不一致
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	bool ColumnAttrCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);
    	/******************************************************************************
    	* 函数名称	:  IndexCmp
    	* 函数描述	:  比较两个表的属性是否一致
    	* 输入		:  
    	* 输入		:  
    	* 输出		:  
    	* 返回值	:  true 一致;false 不一致
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	bool IndexCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);

    	/******************************************************************************
    	* 函数名称	:  VerifySysCfg()
    	* 函数描述	:  校验DSN配置文件参数
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  成功返回0,否则返回-1
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	int VerifySysCfg();
    	/******************************************************************************
    	* 函数名称	:  CheckRepeatTableName
    	* 函数描述	:  判断上载的表中是否存在重复的表名
    	* 输入		:  
    	* 输入		:  
    	* 输出		:  
    	* 返回值	:  true 存在;false 不存在
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	bool CheckRepeatTableName(const char* pTableName);
    	int CheckColumPosIsContinuous(TMdbTable* pTable);
		
		int CheckIndexValid(TMdbIndex& tIndex);

        int LoadTableSpaceAlterInfo(const char* psSysFilePath);

        int GetTableNameFromDir(const char* psDir, char* psTableName, int iNameLen);
        int GetIdleTableId();
        int LoadTableAlterInfo(const char* psCfgDir,const char* psTabName);
        int LoadTableOldConfig();
        int LoadTableOldStruct(const char* psCfgDir,const char* psTabName);

        int GetTableLevelCnt(int iTabLevel);
        //读取不加载的配置
        void LoadFromDBCfg(const char *sAttrValue,std::vector<std::string> &vLoadFromDB);
    public:        

        
        //根据配置觉得加载方式，true 表示不加载
        bool IsNotLoadFromDB(const char* pTableName);

        // 是否存在新增的表空间
        bool HaveNewTableSpace();
        
        // 判断该表空间是否是新增的表空间
        bool IsNewTableSpace(const char* psTsName);

        // 是否存在删除的表空间
        bool HaveDelTableSpace();

        // 判断该表空间是否已删除
        bool IsDelTableSpace(const char* psTsName);

        // 是否存在pagesize变更的表空间
        bool HavePageSizeAlterTS();

        // 判断该表空间是否变更了pagesize，如有，pAlterAttr返回相关的变更信息
        bool IsPageSizeAlter(const char* psTableSpaceName,TIntAlterAttr* pAlterAttr);

        // 表是否有影响加载的表变更
        bool IsTableAlter(const char* psTableName);

        bool IsColumnAlter(const char* psTableName, const char* psColmName);

        // 表所属的表空间是否改变,如有，pTsAlterInfo返回相关的变更信息
        bool IsTableSpaceChange(const char* psTableName, TStringAlterAttr* pTsAlterInfo);

        // 表是否有新增列
        bool HaveNewColumn(const char* psTableName);

		// 表是否有删除列
		bool HaveDropColumn(const char* psTableName);

		int GetDropColumnIndex(const char* sTableName, int * iDropIndex);

		// 判断某表的某列是否为删除的列
		bool IsDropColumn(const char* psTableName, const char* psColmName);
		
        // 判断某表的某列是否是新增的列
        bool IsNewColumn(const char* psTableName, const char* psColmName);

        // 判断列的datatype是否有变更，如有，pAlterInfo返回变更信息
        bool IsColumnDataTypeAlter(const char* psTableName, const char* psColumnName, TIntAlterAttr* pAlterInfo);

        // 判断列的data length是否有变更，如有，pAlterInfo返回变更信息
        bool IsColumnDataLenAlter(const char* psTableName, const char* psColumnName, TIntAlterAttr* pAlterInfo);

        int GetTabLevelByRecordCounts(int iRecdCnts);
        int GetRecordCountsByTabLevel(int iTabLevel);
        bool IsDsnSame(const char* sName);
        //oracle 二次登录校验
		int ReLoadOracle();
	
    public:
        std::vector<TMdbJob>  m_vMdbJob;//job列表
    protected:

    private:
        char m_sCfgFile[MAX_PATH_NAME_LEN]; //系统配置文件
        char m_sTabCfgFile[MAX_PATH_NAME_LEN];//业务表配置文件
        char m_szHome[MAX_PATH_NAME_LEN];

        char m_sDSN[MAX_NAME_LEN];          //DSN名称
        int  m_iUserCounts;               //用户的个数
        int  m_iTableCounts;                //表的个数
        int  m_iTableSpaceCounts;           //表空间个数
        int m_iSeqCounts;                 //序列个数
        TMdbCfgDSN m_tDsn;					//配置文件的结构表示--QuickMDB_SYS_XXX.xml
        TMdbCfgProAttr m_tProAttr;
        TMdbTable* m_pTable[MAX_TABLE_COUNTS]; //表结构指针
        //TMdbTable* m_pOldTabStruct[MAX_TABLE_COUNTS]; // 旧的表结构指针，从上次创建完成后备份的表配置文件读取
        std::vector<TMdbTable*> m_vpOldTabStruct;
        TMdbTableSpace* m_pTableSpace[MAX_TABLE_COUNTS]; //表空间结构指针
        TMemSeq *m_pSeq[MAX_SEQUENCE_COUNTS];   //序列的结构指针

        TMDBDBInterface* m_pDBLink;   //链接
        TMDBDBQueryInterface*    m_pQuery, *m_pQueryC, *m_pQueryI, *m_pQueryTemp;    //动作
        char m_cAccess;
        bool m_bCreateFlag;
        TMDbUser* m_pUser[MAX_USER_COUNT];
        bool m_bStartFlushFromDbProc;//是否需要启动从Oracle同步的进程
        bool m_bStartDbRepProc;
        bool m_bStartShardBackupProc;
        bool m_bStartFileStorageProc;
    	char m_sOnsiteFile[MAX_PATH_NAME_LEN];
        TMdbTsAlterInfo m_tTsAlterInfo; // 表空间配置文件变更信息
        std::vector<TMdbTabAlterInfo> m_vTableAlterInfo; // 表配置文件变更信息


    };

    //配置文件管理类
    class TMdbConfigMgr
    {
    public:
    	TMdbConfigMgr();
    	~TMdbConfigMgr();
    	static TMdbConfig * GetMdbConfig(const char * sDsn);//获取配置文件
    	static TMdbConfig * GetDsnConfig(const char * sDsn);//获取配置文件
    private:
    	static TMdbConfig * m_pArrMdbConfig[MAX_DSN_COUNTS];
    	static TMdbConfig * m_pArrDsnConfig[MAX_DSN_COUNTS];
    	static TMutex  m_tMutex;//互斥锁
    public:
        class CGarbo //它的唯一工作就是在析构函数中删除CSingleton的实例
        {
                public:
                    ~CGarbo()
                    {
                        int i = 0;
                        for(i = 0;i < MAX_DSN_COUNTS;++i)
                        {
                            if( TMdbConfigMgr::m_pArrMdbConfig[i])
                            {
                                delete TMdbConfigMgr::m_pArrMdbConfig[i];
                                TMdbConfigMgr::m_pArrMdbConfig[i] = NULL;
                            }
    						if( TMdbConfigMgr::m_pArrDsnConfig[i])
                            {
                                delete TMdbConfigMgr::m_pArrDsnConfig[i];
                                TMdbConfigMgr::m_pArrDsnConfig[i] = NULL;
                            }
                        }
                    }
        };
    private:
        static CGarbo Garbo; //定义一个静态成员，程序结束时，系统会自动调用它的析构函数
    };

//}

#endif //__ZTE_MINI_DATABASE_CONFIG_H__

