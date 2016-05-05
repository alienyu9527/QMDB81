/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbConfig.h		
*@Description�� ͳһ���÷���������ļ�
*@Author:		jiang.lili
*@Date��	    2014/03/20
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
	 * @brief ������Ϣ
	 * 
	 */
    class TMdbRepHost
    {
    public:
        int m_iHostID; //����ID��Ψһ��ʶһ̨����
        std::string m_strIP; //����IP
        int m_iPort; //�����˿ں�
    };

     /**
	 * @brief ·�ɹ�����Ϣ
	 * 
	 */
    class TMdbRepRules
    {
    public:
        int m_iRuleID; //·�ɹ����ʶ
        std::string m_strRule; //һ��·����Ϲ��򣬿��Ա�ʾһ����Χ��·�ɣ�����һ��·���б��ɶ��ŷָ���
    };

    class TMdbRoutingCheck//У���õ�·�ɹ���ṹ
    {
    public:
        int iStart;//·�ɿ�ʼ
        int iEnd;//·�ɽ���
    };

     /**
	 * @brief ����Ϣ
	 * 
	 */
    class TMdbGroup
    {
    public:
        //int m_iGroupID;
        std::vector<int> m_arHostIDList;//�����б�
        std::vector<int> m_arRuleIDList; //·�ɹ����б�
        std::map<int, std::vector<int> > m_tRoutingMap;//·�ɺ�������ӳ���ϵ
    };

     /**
	 * @brief ��������
	 * 
	 */
    class TMdbDisasterHost
    {
    public:
        //int m_iGroupID;
        int m_iHostID;
        std::string m_strRulelist;//·�ɹ����б�
    };

     /**
	 * @brief �����ļ�������
	 * 
	 */
    class TMdbServerConfig
    {
    public:
        TMdbServerConfig();
        ~TMdbServerConfig();

        /******************************************************************************
        * ��������	:  ReadConfig
        * ��������	: ��ȡ�����ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int ReadConfig();
        /******************************************************************************
        * ��������	:  RereadConfig
        * ��������	: ���¶�ȡ�����ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int RereadConfig();

        /******************************************************************************
        * ��������	:  SyncConfig
        * ��������	: ���ݱ�����ͬ�������ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SyncLocalCfgFile(const char* pData);

        /******************************************************************************
        * ��������	:  GetConfigData
        * ��������	: ��ȡ�����ļ�����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetConfigData(char* pDataBuf);

        /******************************************************************************
        * ��������	:  GetRoutingRequestData
        * ��������	: ��ȡ·����������
        * ����		:  iHostID ������ʶ
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetRoutingRequestData(int iHostID, int iBufLen, char* pDataBuf);

        /******************************************************************************
        * ��������	:  UpdateRouterRep
        * ��������	: ���������ļ�·����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int UpdateRouterRep();

        /******************************************************************************
        * ��������	:  GetHostID
        * ��������	:  ����IP�Ͷ˿ںţ���ȡ����ID
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetHostID(const char* pszIP, int iPort);

        /******************************************************************************
        * ��������	:  GetRouters
        * ��������	:  ����IP�Ͷ˿ںţ���ȡ����ID
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetRouters(int iHostID);

        /******************************************************************************
        * ��������	:  GetHearbeatWarning
        * ��������	:  ��ȡ�������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetHearbeatWarning(){return m_iHeartBeatWarning;}

        /******************************************************************************
        * ��������	:  GetPort
        * ��������	:  ��ȡ���÷���˿ں�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        TMdbRepHost* GetLocalHost();

         /******************************************************************************
        * ��������	:  GetRepServer
        * ��������	:  ��ȡ���÷��񱸻�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        TMdbRepHost * GetRepServer();


    private:
        int LoadHosts(tinyxml2::XMLElement* pRoot);
        int LoadRules(tinyxml2::XMLElement* pRoot);
        int LoadGroups(tinyxml2::XMLElement* pRoot);
        int LoadDisaster(tinyxml2::XMLElement* pRoot);
        int LoadSys(tinyxml2::XMLElement* pRoot);

        void Clear();//����Ѿ���ֵ�ı���

        int GetGroupIndex(int iHostID);//��ȡ����ID��������

        int CheckRoutingRule(const char* sRules);//���·�ɷ����Ƿ�Ϸ�
        bool CheckRange(int iStart, int iEnd, std::vector<TMdbRoutingCheck> &tRoutingCheck);//���·�ɷ�Χ�Ƿ񽻲�
        bool CheckRoutingID(std::vector<int>& vRoutingID);//������е�·��ID�Ƿ�Ϸ�
        bool CheckHostID(std::vector<int>& vHostID);//������е�����ID�Ƿ�Ϸ�
        bool CheckRoutingRep(int iRuleID, std::vector<int>&vHostID, TMdbGroup &tGroup);//���·��ӳ���Ƿ�Ϸ�
        bool CheckHostInfo(const char* sIP, int iPort);//���������Ϣ�Ƿ����ظ�

        
    public:
        tinyxml2::XMLDocument* m_ptDoc;//�����ļ�ָ��
        std::vector<TMdbGroup> m_arGroup;//��������Ϣ
        std::vector<TMdbDisasterHost> m_arRecoveryHost;//���������б�
        std::map<int, TMdbRepHost> m_AllHosts;//���е�������Ϣ������host_idӳ��
        std::map<int, TMdbRepRules> m_AllRules;//���е�·�ɹ�����Ϣ������rule_idӳ��

        int m_iRepHostID; //���÷��񱸻�ID
        int m_iLoaclHostID;//�������÷����ID
        int m_iHeartBeatWarning;//�������
        int m_iHeartBeatFatal;//�����쳣���
        //int m_iPort;//���÷���˿ں�
    };

     // ��Ƭ���ݱ���������Ϣ������
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
        int m_iHostID;//����ID
        int m_iHeartbeat;//�������
        std::map<int, std::vector<int> > m_mRoutingMap;//·�ɺ�������ӳ���ϵ    
        std::vector<TMdbDisasterHost*> m_vRecoveryHost;//���������б�
        std::map<int ,TMdbRepRules*> m_mRuleMap; //���е�·�ɹ�����Ϣ
        std::map<int ,TMdbRepHost*> m_mHostMap;//���е�������Ϣ
    };

     
//}


#endif

