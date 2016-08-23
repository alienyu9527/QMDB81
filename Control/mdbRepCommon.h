/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepCommon.h		
*@Description: �����Ƭ�������õĽṹ��ͱ���
*@Author:		jiang.lili
*@Date��	    2014/05/4
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


#define IS_DISASTER_RECOVERY "1" //QuickMDB�������ֻ��� �����÷���ͨ�ű�ʶ
#define NOT_DISASTER_RECOVERY "0" //QuickMDB���������ֻ��������÷���ͨ�ű�ʶ
#define REP_ROUTING_ADD "A" //����·�ɲ����������÷���ͨ�ű�ʶ
#define  REP_ROUTING_DELETE "D" //ɾ��·�ɲ����������÷���ͨ�ű�ʶ

#define  MAX_REP_ROUTING_LIST_LEN 1024 //·��ID������󳤶�
#define  MAX_REP_HOST_COUNT 10 //һ��·�����ı�����������
#define MAX_REP_ROUTING_ID_COUTN 1000//һ̨������·��ID��������
#define  MAX_ALL_REP_HOST_COUNT 100 //һ̨������Ӧ����󱸻�����
#define  MDB_SHM_ROUTING_REP_NAME "routing_rep" //·��ӳ���ϵ�����ڴ�����
#define  MDB_REP_FILE_PATTEN "Rep.*.OK" //ͬ���ļ���ʽ
#define  MDB_REP_EMPTY_HOST_ID -1 //δ��ֵ��HostID
#define  MDB_MAX_REP_SERVER_THREAD 10//server�˿����������������߳���
#define  MDB_MAX_REP_FILE_LEN 1024*1024 //ͬ���ļ�����󳤶�
#define  MDB_MAX_REP_FILE_BUF_LEN 32*1024 //ͬ���ļ�д����������󳤶�

#define MAX_ONLINE_REP_BUF_USE_PERCENT 50 //����ͬ��ģʽ��������·����ռ����

const char MDB_REP_RCD_BEGIN[]= "^^";//ͬ����¼�Ŀ�ʼ
const char MDB_REP_RCD_END[] = "##"; //ͬ����¼�Ľ���

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
	 * @brief ���ݿ�״̬
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
	 * @brief ����ص�QuickMDB��״̬
	 * 
	 */
    class TMdbRepState:public TMdbNtcBaseObject
    {
    public:
        int m_iHostID;//������ʶ
        EMdbRepState m_eState; //��ǰ״̬
       // int m_iPno; //PID     
    };

    /**
	 * @brief ������Ϣ
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
        int m_iHostID; //������ʶ
        int m_iPort; //�˿ں�
        char m_sIP[MAX_IP_LEN]; //IP
        bool m_bSameEndian;  //�����뱾���ֽ����Ƿ�һ��
    };

     /**
	 * @brief ·���뱸���Ķ�Ӧ��ϵ
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
        int m_iRoutingID;//·��ID
        int m_aiHostID[MAX_REP_HOST_COUNT];//��Ӧ�����б����������ȷ�����ȼ�
        int m_iRecoveryHostID; //���ֻ�
    };

    /**
	 * @brief ����·���뱸��ӳ���ϵ������
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
		
        bool m_bNoRtMode; // ������·�ɵ�ģʽ,������֧�־ɰ汾������ͬ��ģʽ
        //bool m_bRunning;//mdbRep�Ƿ���������
        bool m_bRoutingChange;//�Ƿ�����·�ɱ��
        EMdbRepState m_eState;
        int m_iHostID; //������Ӧ��HostID
        int m_iHeartbeat;//�������
        int m_iRepHostCount;//����������
        int m_iRoutingIDCount;//·������
        int m_iLocalPort;//������repServer�Ķ˿ں�
        char m_sRoutingList[MAX_REP_ROUTING_LIST_LEN];//������·���б�
        char m_sFailedRoutingList[MAX_REP_ROUTING_LIST_LEN];//����ʧ�ܵ�·���б�
        TMdbShmRepHost m_arHosts[MAX_ALL_REP_HOST_COUNT]; //������Ӧ�����б���
        TMdbShmRepRouting m_arRouting[MAX_REP_ROUTING_ID_COUTN]; //������·�����䱸�����������ֻ����Ķ�Ӧ��ϵ
        bool m_bNetConnect[MAX_ALL_REP_HOST_COUNT];
		int m_iRepMode[MAX_ALL_REP_HOST_COUNT];
    };

     /**
	 * @brief ����·���뱸��ӳ���ϵ�����ڴ������
	 * 
	 */
    class TMdbShmRepMgr
    {
    public:
        TMdbShmRepMgr(const char* sDsn);
        ~TMdbShmRepMgr();
    public:
        /******************************************************************************
        * ��������	:  Create
        * ��������	:  ���������ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Create();
        /******************************************************************************
        * ��������	:  Attach
        * ��������	:  ���ӹ����ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	: 0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Attach();
        /******************************************************************************
        * ��������	:  Detach
        * ��������	:  �Ͽ��빲���ڴ������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Detach();
        /******************************************************************************
        * ��������	:  Destroy
        * ��������	:  ���ٹ����ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Destroy();

        /******************************************************************************
        * ��������	:  WriteRoutingInfo
        * ��������	:  ��·����Ϣд�빲���ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int WriteRoutingInfo(const char* pData, int iLen);

        /******************************************************************************
        * ��������	:  SetHostID
        * ��������	:  ���ñ�����Ӧ��HostID
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SetHostID(int iHostID)
        {
            CHECK_OBJ(m_pShmRep);
            m_pShmRep->m_iHostID = iHostID;
            return 0;
        }
        /******************************************************************************
        * ��������	:  SetHeartbeat
        * ��������	:  �����������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SetHeartbeat(int iHeartbeat)
        {
            CHECK_OBJ(m_pShmRep);
            m_pShmRep->m_iHeartbeat = iHeartbeat;
            return 0;
        }
        /******************************************************************************
        * ��������	:  SetMdbState
        * ��������	:  ����MDB״̬
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SetMdbState(EMdbRepState eState)
        {
            CHECK_OBJ(m_pShmRep);
            m_pShmRep->m_eState = eState;
            return 0;
        }
        /******************************************************************************
        * ��������	:  SetRunningFlag
        * ��������	:  ��ʶmdb�Ƿ�������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SetRunningFlag(bool bRunning)
        {
            CHECK_OBJ(m_pShmRep);
            //m_pShmRep->m_bRunning = bRunning;
            return 0;
        }

        /******************************************************************************
        * ��������	:  GetRoutingRep
        * ��������	:  ��ȡ�����ڴ������ָ��
        * ����		:  
        * ���		:  
        * ����ֵ	: 
        * ����		:  jiang.lili
        *******************************************************************************/
        const TMdbShmRep *GetRoutingRep()
        {
            return m_pShmRep;
        }

       /******************************************************************************
        * ��������	:  GetRepHosts
        * ��������	:  ����·��ID����ȡ���Ӧ�����б���
        * ����		:  
        * ���		:  
        * ����ֵ	:  
        * ����		:  jiang.lili
        *******************************************************************************/
        TMdbShmRepRouting* GetRepHosts(int iRoutingID);

       /******************************************************************************
        * ��������	:  AddFailedRoutingList
        * ��������	:  ������ʧ�ܵ�·���б�д�빲���ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int AddFailedRoutingList(const char* sRoutingList);

         /******************************************************************************
        * ��������	:  AddFailedRoutingID
        * ��������	:  ������ʧ�ܵ�·��IDд�빲���ڴ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int AddFailedRoutingID(int iRoutingID);

       /******************************************************************************
        * ��������	:  GetFailedRoutingList
        * ��������	:  ��ȡ����ʧ�ܵ�·��ID�б�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
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
        static bool IsIDInString(int iID, const char* sIDList);//ID�Ƿ���ID�б���
        static bool IsIDInArray(int iID, std::vector<int>&tIDArray);//ID�Ƿ���������
        static int GetIntArrayFromStrList(const char* sIDList, std::vector<int> &tIDArray);//����ID�б���ID����
        static int GetIDArrayFromRule(const char* sRule, std::vector<int>& tIDArray);//���ݹ��򣬲���RoutingID����
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
        std::string m_sServerIP;//���÷���IP
        int m_iServerPort;//���÷���˿ں�

        std::string m_sRepServerIP;//���÷��񱸻�IP
        int m_iRepServerPort;//���÷��񱸻��˿ں�

        std::string m_sRepPath;//ͬ���ļ���Ŀ¼
        time_t m_iFileInvalidTime;//ͬ���ļ��Ĺ���ʱ��
        int m_iMaxFileSize;//ͬ���ļ�����С
        int m_iBackupInterval;//ͬ���ļ����ݼ��

        std::string m_sLocalIP;//����IP
        int m_iLocalPort;//DSN repServer ʹ�õĶ˿ں�

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
		int m_iColType;//�������е�����
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
		int m_iIsLocalCol[MAX_COLUMN_COUNTS]; //0-�����У�1-�Զ��У�-1-û����
		int m_iColPos[MAX_COLUMN_COUNTS];//�����е��кţ��Զ��ж�Ӧ��change����
		int m_iColCount;
	};
//}
#endif
