/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepLoadCtrl.h		
*@Description: ��Ƭ���ݵ���������ģ��
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_LOAD_DATA_CTRL_H__
#define __ZTE_MINI_DATABASE_REP_LOAD_DATA_CTRL_H__
#include "Interface/mdbQuery.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepNTC.h"


//namespace QuickMDB
//{
    class TMdbRepLoadThread:public /*QuickMDB::*/TMdbNtcThread
    {
    public:
        TMdbRepLoadThread(TMdbRepDataClient *pClient=NULL);
        ~TMdbRepLoadThread();

    public:
        int AddRoutingID(int iRoutingID);//������Ҫ���ص�·��ID
        int GetHostID();//��ȡ��Ӧ�ı���ID
        int SetRuningInfo(TMdbConfig* pMdbCfg);//�����߳�����ʱ��Ϣ

    private:
        int LoadDataFromRepHost(const char*sRoutinglist);//�ӱ�����������
        int LoadDataFromStorage(const char* sRoutinglist);//�������洢��������

    protected:
        virtual int Execute();

    private:
        int m_iLocHostID;
        int m_iRepHostID;
        TMdbConfig *m_pMdbCfg;
        std::vector<int> m_aiRoutingID;
        TMdbRepDataClient *m_pClient;//������Ӧ�����ӣ��Ӵ洢����ʱ����ֵΪNULL             
    };

    

    class TMdbRepLoadDataCtrl
    {
    public:
        TMdbRepLoadDataCtrl();
        ~TMdbRepLoadDataCtrl();

    public:
        int Init(const char* sDsn, TMdbConfig* pMdbCfg);

        // �����÷��������״̬�¼�������
        int LoadData();
        // �����÷����δ����״̬�¼�������
        int LoadDataNoSvr();

    private:
        int ConnectRepHosts();//���������б����������ݵ��߳�
        int DisConnectRepHosts();//�Ͽ������б���������
        int DealLeftFile();//��������������
        int LoadDataFromRep();//�ӱ�����������

        int CreateLoadThreads();//Ϊÿ��·�ɴ������߷�����ص��߳�

    private:
        bool IsFileOutOfDate(time_t tTime);//�ļ��Ƿ����
        TMdbRepDataClient * GetClient(int iHostID);//��ȡ������Ӧ������
        TMdbRepLoadThread *GetThread(int iHostID);//��ȡ������Ӧ�ļ����߳�

        int LoadDataFromOracle(int iRouting_ID);//��oracle��������
        int LoadDataFromFile(int iRouting_ID);//���ļ���������
        int LoadDataFromMySql(int iRouting_ID);//��MySql�м�������

        // �ӱ��������ļ���ȡ��Ƭ����������Ϣ�������ڴ���
        int WriteLocalRoutingInfo();
        // �������ڴ������µķ�Ƭ����������Ϣ���浽���������ļ�
        int SyncLocalConfig();

    private:
        TMdbRepConfig *m_pRepConfig;//���÷����������
        TMdbShmRepMgr *m_pShmMgr;//�����ڴ������ָ��
        TMdbRepMonitorClient *m_pMonitorClient;       
        const TMdbShmRep *m_pShmRep;//·����Ϣ�����ڴ�

        TMdbRepDataClient **m_arpLoadClient;
        /*QuickMDB::*/TMdbNtcAutoArray m_arThread;
         std::string m_strDsn;//dsn����

         TMdbConfig* m_pMdbCfg;

         bool m_bSvrConnFlag; // �Ƿ������Ϸ�Ƭ�������÷����

         int m_iHostID; //����ID
       
    };

    

//};
#endif