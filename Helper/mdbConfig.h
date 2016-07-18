/****************************************************************************************
*@Copyrights  2007�����������Ͼ�����������޹�˾ ������ CCB��Ŀ��
*@                   All rights reserved.
*@Name��	    TMdbConfig.h
*@Description�� minidb�������ļ�������
*@Author:		li.shugang
*@Date��	    2008��11��25��
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
#include <unistd.h>  //access����
#else
#include <stdio.h>   //access����
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

    //mdb NULLֵ����
    class TMdbNULLManager
    {
    public:
        static int CalcNullSize(TMdbTable * pTable);//��ȡ�ñ���Ҫ��NULLsize
        static bool IsColumnNULL(char * pAddr,int iColumnPos);//�жϸ����Ƿ�ΪNULL
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

    //�����ļ��Ľṹ��ʾ--QuickMDB_SYS_XXX.xml
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
        //int  iValue;              //DSN��Ӧ��ֵ�����ǵ���������ݿ����г�ͻ
        long long llValue; //����DSN ��QuickMDB_HOME �����hashֵ���滻iValue
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
        char sStorageDir[MAX_PATH_NAME_LEN];//�ļ��洢λ��

        char sOracleID[MAX_NAME_LEN];
        char sOracleUID[MAX_NAME_LEN];
        char sOraclePWD[MAX_NAME_LEN];

        char sLocalIP[MAX_IP_LEN];   //����IP
        char sLocalIP_active[MAX_IP_LEN];   //����IP,����1.2��local_active_ip
        char sPeerIP[MAX_IP_LEN];    //�Զ�IP
        char sActiveIP[MAX_IP_LEN];  //���ֻIP
        char sStandByIP[MAX_IP_LEN]; //����standbyIP
        int  iLocalPort;             //���ض˿�
        int  iPeerPort;              //�Զ˶˿�
        int  iActivePort;            //���ֻ�˿�
        int  iStandbyPort;             //����standby�˿�
		char sAgentPortStr[64]; 	 //����˿��б��ַ���
		int  iAgentPort[MAX_AGENT_PORT_COUNTS]; 			//����˿��б�
		
		char sNtcPortStr[64];		 //ʹ��ntc�Ķ˿��б��ַ���
		char sNoNtcPortStr[64];      //��ʹ��ntc�˿��б��ַ���
		int  iNtcPort[MAX_AGENT_PORT_COUNTS];               //ʹ��ntc�Ķ˿��б�
		int  iNoNtcPort[MAX_AGENT_PORT_COUNTS];             //��ʹ��ntc�Ķ˿��б�
		
        int  iLogLevel;              //��־����
        bool bIsReadOnlyAttr;        //DSN�ķ�������
        bool bIsOraRep;              //�Ƿ���Oracle��ͬ��

		//begin ����1.2
        bool bIsPeerRep;            //�Ƿ������ֱ���
        bool bIsRep;                 //�Ƿ�������ͬ��
        //end
        
        bool bIsCaptureRouter;            //�Ƿ���·�ɲ���
        bool bIsLicense;             //�Ƿ���Ҫ������к�

        int iKillTime;              //ɱ�����̵�ʱ�䣬����Ϊ��λ
        int iRepType;               //1-�첽(�ٶ����); 2-��ͬ��(�ٶȽ���50%,���Ǹ��Ӱ�ȫ); 3-ȫͬ��(�ٶ����������ǰ�ȫ�����)
        int iOraRepCounts;          //Oracleͬ�����̵ĸ���
        int iNetDropTime;           //����Ķ���ʱ�䣬����Ϊ��λ�����ʵ��ʱ��С��X��(���ϵĸ���)������Ϊ���Լ����������ݣ�����ȫ��ͬ������
        int iDelayTime;         //���ݴ�Oracle���ڴ����ݿ���ӳ�ʱ��,��λΪ��
        int iCleanTime;         //����%DSN%_MDB_CHANGE_NOTIF �����ݵ�ʱ��,��λmin
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
        int iClientTimeout;//CSģʽ�Ŀͻ����ṩ��ʱ���ƣ���λ��
        char sRoutingName[MAX_NAME_LEN]; //·��ID������
        char sRoutingList[MAX_ROUTER_LIST_LEN];//DSN�����ݶ�Ӧ��·���б�[ur: 415074 ����]
        int m_iOraRepInterval; // ORACLE2MDBˢ��ʱ��������λ��
        int m_iOraRepDelaySec; // ORACLE2MDB ©ˢʱ�䣬 ��λ��
        int m_iSeqCacheSize;  //������createʱcache�Ĵ�С
        bool m_bIsDiskStorage;//�Ƿ������̴洢
        int m_iLongSqlTime;//��ʱ���sql
        int m_iRepFileTimeout;//����ͬ���ļ���ʱ��Ĭ��24Сʱ����λ : Сʱ
        int m_iCSPumpInitCount;//CSģʽʹ���¼��ó�ʼ����
        int m_iCSPumpMaxCount;//�¼�������
        int m_iCSPeerCountPerPump;//ÿ���¼��ô����peer����
		char  m_sCSValidIP[MAX_IP_LEN*16+1];//��Чcs ip
		char  m_sCSInValidIP[MAX_IP_LEN*16+1];//��Чcs ip
        
        bool m_bShadow; // ORACLE2MDBͬ���Ƿ���Ӱ�ӱ�ģʽ
        bool m_bSingleDisaster;  //�Ƿ񵥻����ֲ���
        bool m_bNull;//����null�������:true ��ʾ��nil�������㻹Ϊnil;false ��ʾnil����ʱ��0ֵ����(�����ϰ汾)��        
        bool m_bIsShardBackup;		 //�Ƿ�����Ƭ����
        bool m_bUseNTC;//CS�Ƿ�ʹ��NTC

        // ��Ƭ�������������
        char sRepSvrIp[MAX_IP_LEN];
        char sRepStandbySvrIp[MAX_IP_LEN];
        int iRepSvrPort;
        int iRepStandbySvrPort;
        //char sRepLocalIp[MAX_IP_LEN];
        bool m_bSQLIsAnswer;//mdbSQL ִ���ش�������Ƿ���Ҫ����,Ĭ�Ͻ���
        int iRepLocalPort;
        int iInvalidRepFileTime;
        char sRepFilePath[MAX_PATH_NAME_LEN];
        std::vector<std::string> m_vNotLoadFromDBTab;//����Ҫ�����ݿ���صı�
        std::string m_strNotLoadFromDBList;//δ�ָ��ı��б�
        // �������ȼ�
        int m_iLoadPriority; // 1 - file storage first; 2- db storage first;

		bool m_bReloadOra;//�Ƿ����oracle ���ε�½У��
    	bool m_bReloadEncrypt;//���ε�½�Ƿ����
    	char m_sReloadCfgName[MAX_PATH_NAME_LEN];//���ε�½�����ļ�
    	char m_sReloadDbType[MAX_NAME_LEN];//���ε�½���ݿ�����
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
        * ��������	:  Init()
        * ��������	:  ��ʼ�������ļ�
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Init();

		int ParseAgentPort();
		int ParseNtcAgentPort(char * sPortStr,int * iPortArray);

        /******************************************************************************
        * ��������	:  SetFlag()
        * ��������	:  ���ö�����ʶ���Ƿ���Ҫ����¼��������mdbCtrl����
        * ����		:  bFlag�� �����Ҫ����¼��,��Ϊtrue,����Ϊfalse
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetFlag(bool bFlag);

        /******************************************************************************
        * ��������	:  LoadCfg()
        * ��������	:  װ��ĳ��DSN������
        * ����		:  pszDsn, DSN����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadCfg(const char* pszDsn);

        /******************************************************************************
        * ��������	:  LoadSysCfg()
        * ��������	:  װ��ϵͳ����
        * ����		:  cSysCfgFile, ϵͳ�����ļ�
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadSysCfg(const char* cSysCfgFile,const bool bCheck=true);

        /******************************************************************************
        * ��������	:  LoadTabCfg()
        * ��������	:  װ���û�������
        * ����		:  cTabCfgFile, �û�����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadTabStruct(const char* cTabCfgFile);

        /******************************************************************************
        * ��������	:  LoadTabCfgEx()
        * ��������	:  װ���û�������
        * ����		:  cTabCfgFile, �û�����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadTabProperty(const char* cTabCfgFile);

        /******************************************************************************
        * ��������	:  CheckParam()
        * ��������	:  ����û������������ȷ��
        * ����		:  pszUser, �û���
        * ����		:  pszPWD, ����
        * ����		:  pszDsn, DSN����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int CheckParam(const char* pszUser, const char* pszPWD, const char* pszDsn);

        /******************************************************************************
        * ��������	:  GetTable()
        * ��������	:  ��ȡĳ������Ϣ
        * ����		:  pszTable, ������
        * ���		:  ��
        * ����ֵ	:  �ɹ����ر�ṹָ�룬���򷵻�NULL
        * ����		:  li.shugang
        *******************************************************************************/
        TMdbTable* GetTable(const char* pszTable);

        // ��ȡ�ɵı�ṹ������Ϣ
        TMdbTable* GetOldTableStruct(const char* pszTabName);

        /******************************************************************************
        * ��������	:  GetTableCounts()
        * ��������	:  ��ȡ����Ŀ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���ر���Ŀ
        * ����		:  li.shugang
        *******************************************************************************/
        int GetTableCounts();

        /******************************************************************************
        * ��������	:  GetTableSpaceCounts()
        * ��������	:  ��ȡ��ռ���Ŀ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���ر�ռ���Ŀ
        * ����		:  li.shugang
        *******************************************************************************/
        int GetTableSpaceCounts();

        /******************************************************************************
        * ��������	:  GetUserCounts()
        * ��������	:  ��ȡ�û�����
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �����û���Ŀ
        * ����		:  li.shugang
        *******************************************************************************/
        int GetUserCounts();

        /******************************************************************************
        * ��������	:  SetUserCounts()
        * ��������	:  �����û�����
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetUserCounts(int iCount);

    	void SetTablespaceCounts(int iCount);
    	void SetTableCounts(int iCount);
        /******************************************************************************
        * ��������	:  GetTableByPos()
        * ��������	:  ����λ�û�ȡ����Ϣ
        * ����		:  iPos�����λ��
        * ���		:  ��
        * ����ֵ	:  ���ر���Ϣָ��
        * ����		:  li.shugang
        *******************************************************************************/
        TMdbTable* GetTableByPos(int iPos);
        TMdbTable* GetTableByTableId(int iTableId);

    	TMdbTable* GetTableByIdxName(const char* pszIdxName);

        /******************************************************************************
        * ��������	:  GetTableSpace()
        * ��������	:  ��ȡ��ռ���Ϣ
        * ����		:  iPos����ռ��λ��
        * ���		:  ��
        * ����ֵ	:  ���ر�ռ���Ϣָ��
        * ����		:  li.shugang
        *******************************************************************************/
        TMdbTableSpace* GetTableSpace(int iPos);

    	TMdbTableSpace* GetTableSpaceByName(const char* pszTablespaceName);
    	TMdbTableSpace* GetIdleTableSpace();
    	TMdbTable* GetIdleTable();

        /******************************************************************************
        * ��������	:  GetSeqCounts()
        * ��������	:  ��ȡ���е���Ŀ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �������е���Ŀ
        * ����		:  li.shugang
        *******************************************************************************/
        int GetSeqCounts();

        /******************************************************************************
        * ��������	:  GetSequence()
        * ��������	:  ��ȡ���е���Ϣ
        * ����		:  iPos�����е�λ��
        * ���		:  ��
        * ����ֵ	:  ����������Ϣָ��
        * ����		:  li.shugang
        *******************************************************************************/
        TMemSeq* GetSequence(int iPos);

        /******************************************************************************
        * ��������	:  GetDSN()
        * ��������	:  ��ȡDSN��Ϣ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ����DSN��Ϣָ��
        * ����		:  li.shugang
        *******************************************************************************/
        TMdbCfgDSN* GetDSN();

        /******************************************************************************
        * ��������	:  GetProAttr()
        * ��������	:  ��ȡTMdbCfgProAttr��Ϣ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ����TMdbCfgProAttr��Ϣָ��
        * ����		:  li.shugang
        *******************************************************************************/
        TMdbCfgProAttr* GetProAttr();

        /******************************************************************************
        * ��������	:  IsWriteUnLock()
        * ��������	:  �Ƿ�������OCS_SESSIONһ���Ƶ������/ɾ���ı�
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���򷵻�true�����򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        bool IsWriteUnLock();

        /******************************************************************************
        * ��������	:  GetAccess()
        * ��������	:  ��ȡ���ӵĲ���Ȩ��
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �������ӵĲ���Ȩ��
        * ����		:  li.shugang
        *******************************************************************************/
        char GetAccess()
        {
            return m_cAccess;
        }

        /******************************************************************************
        * ��������	:  GetUser()
        * ��������	:  ��ȡ�û��б�
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �������ӵĲ���Ȩ��
        * ����		:  li.shugang
        *******************************************************************************/
        TMDbUser* GetUser(int iPos)
        {
            return m_pUser[iPos];
        }

    	TMDbUser* GetUser(const char* pszUserName);
        /******************************************************************************
        * ��������	:  CheckLicense()
        * ��������	:  ������к��Ƿ���ȷ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��ȷ�򷵻�true�����򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        bool CheckLicense();

        /******************************************************************************
        * ��������	:  IsAdmin()
        * ��������	:  �жϵ�ǰ�û��Ƿ��ǹ���ԱȨ��
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �Ƿ���true�����򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        bool IsAdmin();

        /******************************************************************************
        * ��������	:  GetDataType()
        * ��������	:  �����ֶ����ͻ�ȡ�ڲ�����
        * ����		:  pszDataTpye �ֶ�����
        * ���		:  ��
        * ����ֵ	:  �ڲ���������
        * ����		:  li.shugang
        *******************************************************************************/
        int GetDataType(const char* pszDataTpye);

        /******************************************************************************
        * ��������	:  GetRepType()
        * ��������	:  ��ȡͬ������
        * ����		:  pszRepTpye ���õ�ͬ������
        * ���		:  ��
        * ����ֵ	:  ת�����ڲ�ͬ������
        * ����		:  li.shugang
        *******************************************************************************/
        int GetRepType(const char* pszRepTpye);

    	/******************************************************************************
    	* ��������	:  GetDataType
    	* ��������	:  ��ȡ�������͵��ַ�����ʾ
    	* ����		:
    	* ����		:
    	* ���		:
    	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	int GetDataType(int iDataType,char *pDataType,const int iLen);
    	
    	/******************************************************************************
    	* ��������	:  GetRepType
    	* ��������	:  ��ȡ����ͬ�����͵��ַ�����ʾ
    	* ����		:
    	* ����		:
    	* ���		:
    	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	int GetRepType(int iRepType,char *pRepType,const int iLen);
    	
        /******************************************************************************
        * ��������	:  GetConfigFileName()
        * ��������	:  ��ȡϵͳ�����ļ���
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ϵͳ�����ļ���
        * ����		:  li.shugang
        *******************************************************************************/
        char * GetConfigFileName();

        /******************************************************************************
        * ��������	:  GetIsStartFlushFromOra()
        * ��������	:  �Ƿ�������oracleͬ�����ݽ���
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �Ƿ���true�����򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        bool GetIsStartFlushFromOra();

        /******************************************************************************
        * ��������	:  GetIsStartOracleRep()
        * ��������	:  ��ȡ�Ƿ���oracleͬ�����ݽ���
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �Ƿ���true�����򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        bool GetIsStartOracleRep();

        bool GetIsStartShardBackupRep();

        bool GetIsStartFileStorageProc();

        /******************************************************************************
        * ��������	:  GetConfigHomePath()
        * ��������	:  ��ȡ�����ļ� homepath
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �����ļ�·��
        * ����		:  li.shugang
        *******************************************************************************/
        char * GetConfigHomePath();
        int GetDbDataShmCounts(int &iDataShmCounts);//��ȡ���ݿ���Ҫ�����ݹ����
        int GetOneTableDataSize(TMdbTable * pTable,long long &llDataSize);//��ȡĳ�ű�����Ҫ�����ݿռ��С
        int GetOneTableSpaceDataSize(char* psTableSpaceName,long long &llDataSize);//��ȡĳ����ռ�����Ҫ�����ݿռ��С
        bool IsDiffToMemTable(TMdbTable * pCfgTable,TMdbTable * pMemTable);//�Ƿ���ڴ��һ��
        
        /******************************************************************************
        * ��������	:  CalcOneRecordSize
        * ��������	:  �������һ����¼�Ĵ�С
        * ����		:  pTable - ��Ҫ����ı�
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  jin.shaohua
        *******************************************************************************/
        int CalcOneRecordSize(TMdbTable * pTable);

    	/******************************************************************************
        * ��������	:  SetColOffset()
        * ��������	:  ����е�ƫ������Ϣ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���óɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int SetColOffset(TMdbTable* pTable);
    	
    	/******************************************************************************
        * ��������	:  IsExist()
        * ��������	:  ��������ļ��Ƿ����
        * ����		:  pszDsn DSN����
        * ���		:  ��
        * ����ֵ	:  ���ڷ���true,���򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        bool IsExist(const char* pszDsn);

        /******************************************************************************
        * ��������	:  HasWhereCond()
        * ��������	:  �ж�һ�ű��Load SQL������Ƿ����where����
        * ����		:  sTableName ����
        * ����		:  sLoadSQL sql���
        * ���		:  ��
        * ����ֵ	:  ���ڷ���true,���򷵻�false
        * ����		:  jiang.lili
        *******************************************************************************/
        bool HasWhereCond(const char* sTableName, const char * sLoadSQL);
    	/******************************************************************************
    	* ��������	:  IsValidTableOrColumnName
    	* ��������	:  �жϱ����������Ƿ�Ϸ�
    	* ����		:  pName����������
    	* ���		:  ��
    	* ����ֵ	:  true �ͷ�;false �Ƿ�
    	* ����		:  cao.peng
    	*******************************************************************************/
    	bool IsValidName(const char *pName);
    	
    	/******************************************************************************
    	* ��������	:  CheckColumnProperty()
    	* ��������	:  У��ָ���еĳ��ȡ�ͬ�����Ե�
    	* ����		:  pTable ���������iColumnPos ������λ�ã�
    	* ���		:  ��
    	* ����ֵ	:  0:�ɹ���!0 : ʧ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	int CheckColumnProperty(TMdbTable* pTable, const int iColumnPos);
    	/******************************************************************************
    	* ��������  :  CheckPKIsIndexed()
    	* ��������  :  ����������Ƿ�����
    	* ����		:  pTable �����
    	* ���		:  ��
    	* ����ֵ    :  �ɹ�����0�����򷵻�-1
    	* ����		:  cao.peng
    	*******************************************************************************/
    	int CheckPKIsIndexed(TMdbTable * pTable);		
		int CheckColumnCfg(TMdbTable * pTable);
    	int LoadOnSiteFile(const char* pszDsn);//����onsite�ļ�
    	TMdbTable* GetTableByName(const char* pszTableName);//ͨ��������ȡ����Ϣ
    	int LoadDsnConfigFile(const char* pszDsn,const bool bCheck);
        static long long GetDsnHashValue(const char *sDsn);//����dsn+QuickMDB_HOME ����hashֵ

        int BackUpConfig();
        int ClearAlterFile();
        
    private:
        /******************************************************************************
        * ��������	:  LoadDsnCfg()
        * ��������	:  ��ȡXML�е�����Դ��Ϣ
        * ����		:  pRoot XML�ļ����ڵ�
        * ���		:  ��
        * ����ֵ	:  �ɹ����ط���true�����򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        bool LoadDsnCfg(MDBXMLElement* pRoot);

        /******************************************************************************
        * ��������	:  LoadDsnInfo()
        * ��������	:  ��ȡ����Դ��Ϣ
        * ����		:  pEle DataSource�ڵ�
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDsnInfo(MDBXMLElement* pEle,const bool bCheck);

        /******************************************************************************
        * ��������	:  GetConfigHomePath()
        * ��������	:  ��ȡsys�ڵ�����Դ��Ϣ
        * ����		:  pEle sys�ڵ�
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadNtcPortsInfo(MDBXMLAttribute* pAttr,MDBXMLAttribute* pAttrValue);
        int LoadSysInfoFromXML(MDBXMLElement* pESys);
        int LoadSysInfo(MDBXMLElement* pEle,const bool bCheck);

        /******************************************************************************
        * ��������	:  LoadTableCfg()
        * ��������	:  ��ȡ �� ����
        * ����		:  pMDB table�ڵ�
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadTableStruct(const char* psTableDir, const char* psTableName);

        /******************************************************************************
        * ��������	:  LoadTableSpaceCfg()
        * ��������	:  ��ȡ ��ռ� ����
        * ����		:  pMDB table-space�ڵ�
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadTableSpaceCfg(MDBXMLElement* pMDB,const bool bCheck);

        /******************************************************************************
        * ��������	:  LoadUser()
        * ��������	:  ��ȡ �û� ����
        * ����		:  pMDB user�ڵ�
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadUser(MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������	:  CheckCfg()
        * ��������	:  �������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  0��ʾ����ok,��0��ʾ���÷Ƿ�
        * ����		:  li.shugang
        *******************************************************************************/
        int CheckCfg();

        /******************************************************************************
        * ��������	:  LoadTableColumn()
        * ��������	:  ���ر���ص�����Ϣ
        * ����		:  pMDB table�ڵ�
        * ���		:  pTable ����Ϣ��bIsInOra �Ƿ���oracle����
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadTableColumn(TMdbTable* pTable, MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������	:  LoadTableIndex()
        * ��������	:  ���ر���ص�������Ϣ
        * ����		:  pMDB ��ڵ�
        * ���		:  pTable ����Ϣ
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadTableIndex(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������  :  LoadRelation()
        * ��������  :  �����ӱ������Ķ�ӳ��ϵ
        * ����	   :  pMDB
        * ���	   :  pChildTable �ӱ���Ϣ
        * ����ֵ    :  ���سɹ�����0,���򷵻�-1
        * ����	   :  li.shugang
        *******************************************************************************/
        //int LoadRelation(TMdbChildTable* pChildTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������	:  LoadTablePrimaryKey()
        * ��������	:  ���ر���ص�������Ϣ
        * ����		:  pMDB pkey�ڵ�
        * ���		:  pTable ����Ϣ
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadTablePrimaryKey(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������	:  LoadSQL()
        * ��������	:  ����SQL ��Ϣ
        * ����		:  pMDB table�ڵ�
        * ���		:  pTable ����Ϣ
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadSQL(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������	:  LoadChildInfo()
        * ��������	:  �����ӱ���Ϣ
        * ����		:  pMDB table�ڵ�
        * ���		:  pTable ����Ϣ
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        //int LoadChildInfo(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������	:  LoadParameter()
        * ��������	:  ���ز�������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadParameter(TMdbTable* pTable,MDBXMLElement* pMDB);

        /******************************************************************************
        * ��������	:  LoadSysTable()
        * ��������	:  ���ϵͳ����Ϣ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadSysTable();

        int LoadTable();

        

        /******************************************************************************
        * ��������	:  LoadDBATables()
        * ��������	:  ���dba_tables
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBATables();

        /******************************************************************************
        * ��������	:  LoadDBAColumn()
        * ��������	:  ���dba_column
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBAColumn();

        /******************************************************************************
        * ��������	:  LoadDBAIndex()
        * ��������	:  ���dba_index
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBAIndex();

        /******************************************************************************
        * ��������	:  LoadDBATableSpace()
        * ��������	:  ���dba_tablespace
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBATableSpace();

        /******************************************************************************
        * ��������	:  LoadDBASequence()
        * ��������	:  ���dba_sequence
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBASequence();

        /******************************************************************************
        * ��������	:  LoadDBAUser()
        * ��������	:  ���dba_user
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBAUser();

        /******************************************************************************
        * ��������	:  LoadDBASession()
        * ��������	:  ���dba_session
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBASession();

        /******************************************************************************
        * ��������	:  LoadDBAResource()
        * ��������	:  ���dba_resource
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBAResource();

        /******************************************************************************
        * ��������	:  LoadDBASQL()
        * ��������	:  ���dba_sql
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���سɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int LoadDBASQL();

    	/******************************************************************************
    	* ��������	:  LoadDBAProcess()
    	* ��������	:  ���dba_process
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ���سɹ�����0,���򷵻�-1
    	* ����		:  cao.peng
    	*******************************************************************************/
    	int LoadDBAProcess();

        int LoadDBADual();//����dual��
        /******************************************************************************
        * ��������	:  SetColumn()
        * ��������	:  ����ϵͳ���ĳһ������Ϣ
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetColumn(TMdbTable* pTable, int iCPos, const char* pszColumnName, int iDataType, int iDataLen);

        /******************************************************************************
        * ��������	:  GetLincese()
        * ��������	:  ��ȡ�����ļ� homepath
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void GetLincese(char* pLicense);

        /******************************************************************************
        * ��������	:  SetConfigHomePath()
        * ��������	:  ���������ļ�home·��
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ���óɹ�����0,���򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int SetConfigHomePath(const char* pszDsn);
        int LoadMdbJob();//����mdbjob����
        /******************************************************************************
    	* ��������	:  AddRouteIDForLoadSQL()
    	* ��������	:  Ϊ��ˢ��SQL���·��
    	* ����		:  pTable ��
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	void AddRouteIDForLoadSQL(TMdbTable* pTable);

    	/******************************************************************************
    	* ��������	:  GetTablePropertyValue()
    	* ��������	:  ��ȡ �� ���ã���ȡָ�����Ե�ֵ
    	* ����		:  pMDB table�ڵ�,pPropertyName ������
    	* ���		:  ��
    	* ����ֵ	:  ����ֵ
    	* ����		:  cao.peng
    	*******************************************************************************/
    	char * GetTablePropertyValue(MDBXMLElement* pMDB,const char* pPropertyName);
    	/******************************************************************************
    	* ��������	:  PrimaryKeyCmp
    	* ��������	:  �Ƚ�������������Ƿ�һ��
    	* ����		:  
    	* ����		:  
    	* ���		:  
    	* ����ֵ	:  true һ��;false ��һ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	bool TablePropertyCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);
    	/******************************************************************************
    	* ��������	:  PrimaryKeyCmp
    	* ��������	:  �Ƚ�������������Ƿ�һ��
    	* ����		:  
    	* ����		:  
    	* ���		:  
    	* ����ֵ	:  true һ��;false ��һ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	bool PrimaryKeyCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);
    	/******************************************************************************
    	* ��������	:  ColumnAttrCmp
    	* ��������	:  �Ƚ�����������Ƿ�һ��
    	* ����		:  
    	* ����		:  
    	* ���		:  
    	* ����ֵ	:  true һ��;false ��һ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	bool ColumnAttrCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);
    	/******************************************************************************
    	* ��������	:  IndexCmp
    	* ��������	:  �Ƚ�������������Ƿ�һ��
    	* ����		:  
    	* ����		:  
    	* ���		:  
    	* ����ֵ	:  true һ��;false ��һ��
    	* ����		:  cao.peng
    	*******************************************************************************/
    	bool IndexCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable);

    	/******************************************************************************
    	* ��������	:  VerifySysCfg()
    	* ��������	:  У��DSN�����ļ�����
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  �ɹ�����0,���򷵻�-1
    	* ����		:  cao.peng
    	*******************************************************************************/
    	int VerifySysCfg();
    	/******************************************************************************
    	* ��������	:  CheckRepeatTableName
    	* ��������	:  �ж����صı����Ƿ�����ظ��ı���
    	* ����		:  
    	* ����		:  
    	* ���		:  
    	* ����ֵ	:  true ����;false ������
    	* ����		:  cao.peng
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
        //��ȡ�����ص�����
        void LoadFromDBCfg(const char *sAttrValue,std::vector<std::string> &vLoadFromDB);
    public:        

        
        //�������þ��ü��ط�ʽ��true ��ʾ������
        bool IsNotLoadFromDB(const char* pTableName);

        // �Ƿ���������ı�ռ�
        bool HaveNewTableSpace();
        
        // �жϸñ�ռ��Ƿ��������ı�ռ�
        bool IsNewTableSpace(const char* psTsName);

        // �Ƿ����ɾ���ı�ռ�
        bool HaveDelTableSpace();

        // �жϸñ�ռ��Ƿ���ɾ��
        bool IsDelTableSpace(const char* psTsName);

        // �Ƿ����pagesize����ı�ռ�
        bool HavePageSizeAlterTS();

        // �жϸñ�ռ��Ƿ�����pagesize�����У�pAlterAttr������صı����Ϣ
        bool IsPageSizeAlter(const char* psTableSpaceName,TIntAlterAttr* pAlterAttr);

        // ���Ƿ���Ӱ����صı���
        bool IsTableAlter(const char* psTableName);

        bool IsColumnAlter(const char* psTableName, const char* psColmName);

        // �������ı�ռ��Ƿ�ı�,���У�pTsAlterInfo������صı����Ϣ
        bool IsTableSpaceChange(const char* psTableName, TStringAlterAttr* pTsAlterInfo);

        // ���Ƿ���������
        bool HaveNewColumn(const char* psTableName);

		// ���Ƿ���ɾ����
		bool HaveDropColumn(const char* psTableName);

		int GetDropColumnIndex(const char* sTableName, int * iDropIndex);

		// �ж�ĳ���ĳ���Ƿ�Ϊɾ������
		bool IsDropColumn(const char* psTableName, const char* psColmName);
		
        // �ж�ĳ���ĳ���Ƿ�����������
        bool IsNewColumn(const char* psTableName, const char* psColmName);

        // �ж��е�datatype�Ƿ��б�������У�pAlterInfo���ر����Ϣ
        bool IsColumnDataTypeAlter(const char* psTableName, const char* psColumnName, TIntAlterAttr* pAlterInfo);

        // �ж��е�data length�Ƿ��б�������У�pAlterInfo���ر����Ϣ
        bool IsColumnDataLenAlter(const char* psTableName, const char* psColumnName, TIntAlterAttr* pAlterInfo);

        int GetTabLevelByRecordCounts(int iRecdCnts);
        int GetRecordCountsByTabLevel(int iTabLevel);
        bool IsDsnSame(const char* sName);
        //oracle ���ε�¼У��
		int ReLoadOracle();
	
    public:
        std::vector<TMdbJob>  m_vMdbJob;//job�б�
    protected:

    private:
        char m_sCfgFile[MAX_PATH_NAME_LEN]; //ϵͳ�����ļ�
        char m_sTabCfgFile[MAX_PATH_NAME_LEN];//ҵ��������ļ�
        char m_szHome[MAX_PATH_NAME_LEN];

        char m_sDSN[MAX_NAME_LEN];          //DSN����
        int  m_iUserCounts;               //�û��ĸ���
        int  m_iTableCounts;                //��ĸ���
        int  m_iTableSpaceCounts;           //��ռ����
        int m_iSeqCounts;                 //���и���
        TMdbCfgDSN m_tDsn;					//�����ļ��Ľṹ��ʾ--QuickMDB_SYS_XXX.xml
        TMdbCfgProAttr m_tProAttr;
        TMdbTable* m_pTable[MAX_TABLE_COUNTS]; //��ṹָ��
        //TMdbTable* m_pOldTabStruct[MAX_TABLE_COUNTS]; // �ɵı�ṹָ�룬���ϴδ�����ɺ󱸷ݵı������ļ���ȡ
        std::vector<TMdbTable*> m_vpOldTabStruct;
        TMdbTableSpace* m_pTableSpace[MAX_TABLE_COUNTS]; //��ռ�ṹָ��
        TMemSeq *m_pSeq[MAX_SEQUENCE_COUNTS];   //���еĽṹָ��

        TMDBDBInterface* m_pDBLink;   //����
        TMDBDBQueryInterface*    m_pQuery, *m_pQueryC, *m_pQueryI, *m_pQueryTemp;    //����
        char m_cAccess;
        bool m_bCreateFlag;
        TMDbUser* m_pUser[MAX_USER_COUNT];
        bool m_bStartFlushFromDbProc;//�Ƿ���Ҫ������Oracleͬ���Ľ���
        bool m_bStartDbRepProc;
        bool m_bStartShardBackupProc;
        bool m_bStartFileStorageProc;
    	char m_sOnsiteFile[MAX_PATH_NAME_LEN];
        TMdbTsAlterInfo m_tTsAlterInfo; // ��ռ������ļ������Ϣ
        std::vector<TMdbTabAlterInfo> m_vTableAlterInfo; // �������ļ������Ϣ


    };

    //�����ļ�������
    class TMdbConfigMgr
    {
    public:
    	TMdbConfigMgr();
    	~TMdbConfigMgr();
    	static TMdbConfig * GetMdbConfig(const char * sDsn);//��ȡ�����ļ�
    	static TMdbConfig * GetDsnConfig(const char * sDsn);//��ȡ�����ļ�
    private:
    	static TMdbConfig * m_pArrMdbConfig[MAX_DSN_COUNTS];
    	static TMdbConfig * m_pArrDsnConfig[MAX_DSN_COUNTS];
    	static TMutex  m_tMutex;//������
    public:
        class CGarbo //����Ψһ��������������������ɾ��CSingleton��ʵ��
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
        static CGarbo Garbo; //����һ����̬��Ա���������ʱ��ϵͳ���Զ�����������������
    };

//}

#endif //__ZTE_MINI_DATABASE_CONFIG_H__

