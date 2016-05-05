/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbConfig.h		
*@Description： 统一配置服务的配置文件
*@Author:		jiang.lili
*@Date：	    2014/03/20
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_SERVER_CONFIG_H__
#define __ZTE_MINI_DATABASE_SERVER_CONFIG_H__
//#include "BillingSDK.h"
#include "Helper/mdbStruct.h"
#include "Helper/tinyxml2.h"
#include "Control/mdbRepCommon.h"

//namespace QuickMDB
//{
    #define  MDB_SERVER_CONFIG_DIR "$(QuickMDB_HOME)/etc"
    #define  MDB_SERVER_CONFIG_NAME "mdbServer.xml"

    #define MDB_LOCAL_CONFIG_DIR "$(QuickMDB_HOME)/.config"
    #define MDB_LOCAL_CONFIG_FILE "ShardBackupRep.xml"

     /**
	 * @brief 主机信息
	 * 
	 */
    class TMdbRepHost
    {
    public:
        int m_iHostID; //主机ID，唯一标识一台主机
        std::string m_strIP; //主机IP
        int m_iPort; //主机端口号
    };

     /**
	 * @brief 路由规则信息
	 * 
	 */
    class TMdbRepRules
    {
    public:
        int m_iRuleID; //路由规则标识
        std::string m_strRule; //一个路由组合规则，可以表示一个范围的路由，或者一个路由列表（由逗号分隔）
    };

    class TMdbRoutingCheck//校验用的路由规则结构
    {
    public:
        int iStart;//路由开始
        int iEnd;//路由结束
    };

     /**
	 * @brief 组信息
	 * 
	 */
    class TMdbGroup
    {
    public:
        //int m_iGroupID;
        std::vector<int> m_arHostIDList;//主机列表
        std::vector<int> m_arRuleIDList; //路由规则列表
        std::map<int, std::vector<int> > m_tRoutingMap;//路由和主机的映射关系
    };

     /**
	 * @brief 容灾主机
	 * 
	 */
    class TMdbDisasterHost
    {
    public:
        //int m_iGroupID;
        int m_iHostID;
        std::string m_strRulelist;//路由规则列表
    };

     /**
	 * @brief 配置文件管理类
	 * 
	 */
    class TMdbServerConfig
    {
    public:
        TMdbServerConfig();
        ~TMdbServerConfig();

        /******************************************************************************
        * 函数名称	:  ReadConfig
        * 函数描述	: 读取配置文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int ReadConfig();
        /******************************************************************************
        * 函数名称	:  RereadConfig
        * 函数描述	: 重新读取配置文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int RereadConfig();

        /******************************************************************************
        * 函数名称	:  SyncConfig
        * 函数描述	: 根据备机，同步配置文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SyncLocalCfgFile(const char* pData);

        /******************************************************************************
        * 函数名称	:  GetConfigData
        * 函数描述	: 获取配置文件数据
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetConfigData(char* pDataBuf);

        /******************************************************************************
        * 函数名称	:  GetRoutingRequestData
        * 函数描述	: 获取路由请求数据
        * 输入		:  iHostID 主机标识
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetRoutingRequestData(int iHostID, int iBufLen, char* pDataBuf);

        /******************************************************************************
        * 函数名称	:  UpdateRouterRep
        * 函数描述	: 更新配置文件路由信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int UpdateRouterRep();

        /******************************************************************************
        * 函数名称	:  GetHostID
        * 函数描述	:  根据IP和端口号，获取主机ID
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetHostID(const char* pszIP, int iPort);

        /******************************************************************************
        * 函数名称	:  GetRouters
        * 函数描述	:  根据IP和端口号，获取主机ID
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetRouters(int iHostID);

        /******************************************************************************
        * 函数名称	:  GetHearbeatWarning
        * 函数描述	:  获取心跳间隔
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetHearbeatWarning(){return m_iHeartBeatWarning;}

        /******************************************************************************
        * 函数名称	:  GetPort
        * 函数描述	:  获取配置服务端口号
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        TMdbRepHost* GetLocalHost();

         /******************************************************************************
        * 函数名称	:  GetRepServer
        * 函数描述	:  获取配置服务备机
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        TMdbRepHost * GetRepServer();


    private:
        int LoadHosts(tinyxml2::XMLElement* pRoot);
        int LoadRules(tinyxml2::XMLElement* pRoot);
        int LoadGroups(tinyxml2::XMLElement* pRoot);
        int LoadDisaster(tinyxml2::XMLElement* pRoot);
        int LoadSys(tinyxml2::XMLElement* pRoot);

        void Clear();//清空已经赋值的变量

        int GetGroupIndex(int iHostID);//获取主机ID所属的组

        int CheckRoutingRule(const char* sRules);//检查路由分配是否合法
        bool CheckRange(int iStart, int iEnd, std::vector<TMdbRoutingCheck> &tRoutingCheck);//检查路由范围是否交叉
        bool CheckRoutingID(std::vector<int>& vRoutingID);//检查组中的路由ID是否合法
        bool CheckHostID(std::vector<int>& vHostID);//检查组中的主机ID是否合法
        bool CheckRoutingRep(int iRuleID, std::vector<int>&vHostID, TMdbGroup &tGroup);//检查路由映射是否合法
        bool CheckHostInfo(const char* sIP, int iPort);//检查主机信息是否有重复

        
    public:
        tinyxml2::XMLDocument* m_ptDoc;//配置文件指针
        std::vector<TMdbGroup> m_arGroup;//所有组信息
        std::vector<TMdbDisasterHost> m_arRecoveryHost;//容灾主机列表
        std::map<int, TMdbRepHost> m_AllHosts;//所有的主机信息，根据host_id映射
        std::map<int, TMdbRepRules> m_AllRules;//所有的路由规则信息，根据rule_id映射

        int m_iRepHostID; //配置服务备机ID
        int m_iLoaclHostID;//本地配置服务的ID
        int m_iHeartBeatWarning;//心跳间隔
        int m_iHeartBeatFatal;//心跳异常间隔
        //int m_iPort;//配置服务端口号
    };

     // 分片备份本地配置信息操作类
    class TShbRepLocalConfig
    {
    public:
        TShbRepLocalConfig();
        ~TShbRepLocalConfig();
        
        int ReadLocalConfig(const char* psDsn,char* pConfigBuf, int iBufLen);
        int SyncLocalConfig(const char* psDsn,const char* pRepInfo);

    private:
        
        void Clear();
        int LoadSys(tinyxml2::XMLElement* pRoot);
        int LoadHosts(tinyxml2::XMLElement* pRoot);
        int LoadRules(tinyxml2::XMLElement* pRoot);
        int LoadStandBy(tinyxml2::XMLElement* pRoot);
        int LoadDisaster(tinyxml2::XMLElement* pRoot);
        int GenConfigStr(char* pDataBuf, int iBufLen);
        int AnalyRepInfo(const char* pRepInfo);
        int AddSys(MDBXMLElement* pEle, int iHostID, int iHeartBeat);
        int AddHosts(MDBXMLElement* pEle, TMdbRepHost*  pHostInfo);
        int AddRules(MDBXMLElement* pEle, TMdbRepRules*  pRuleInfo);
        int AddStandBy(MDBXMLElement* pEle, int iRuleID,std::vector<int>& vHosts);
        int AddDisaster(MDBXMLElement* pEle, TMdbDisasterHost* pHostinfo);
        
        
    public:
        int m_iHostID;//本机ID
        int m_iHeartbeat;//心跳间隔
        std::map<int, std::vector<int> > m_mRoutingMap;//路由和主机的映射关系    
        std::vector<TMdbDisasterHost*> m_vRecoveryHost;//容灾主机列表
        std::map<int ,TMdbRepRules*> m_mRuleMap; //所有的路由规则信息
        std::map<int ,TMdbRepHost*> m_mHostMap;//所有的主机信息
    };

     
//}


#endif

