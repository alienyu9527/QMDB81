/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepCtrl.h		
*@Description: ��Ƭ���������̹����࣬���������÷���ͨ�ź���������
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_CTRL_H__
#define __ZTE_MINI_DATABASE_REP_CTRL_H__
#include "Interface/mdbQuery.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepNTC.h"

#include "Control/mdbProcCtrl.h"

//namespace QuickMDB
//{
    class TMdbRepCtrl
    {
    public:
        TMdbRepCtrl();
        ~TMdbRepCtrl();

        /******************************************************************************
        * ��������	:  Init()
        * ��������	:  ��ʼ��
        * ����		:  sDsn mdb��Dsn����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(const char* sDsn);

        /******************************************************************************
        * ��������	:  LoadData()
        * ��������	:  ��������, �����Գ���ʹ�ã�ͨ���趨��ͬ��port��ͬһ̨���������������qmdb
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  jiang.lili
        *******************************************************************************/
        // int LoadData(const char* sDsn, int iPort);

        /******************************************************************************
        * ��������	:  LoadData()
        * ��������	:  ��������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  jiang.lili
        *******************************************************************************/
        // int LoadData(const char* sDsn, TMdbConfig* pMdbCfg);

        /******************************************************************************
        * ��������	:  Start()
        * ��������	:  ������Ƭ���ݸ����ӽ��̣���������ͬ����״̬�ϱ���·�ɱ������
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  jiang.lili
        *******************************************************************************/
        int Start();

         /******************************************************************************
        * ��������	:  ReportState()
        * ��������	:  �ϱ�QuickMDB״̬����QuickMDB���ã�����Ҫ����Init������ֱ�ӵ��ü���
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  jiang.lili
        *******************************************************************************/
        static int ReportState(const char*sDsn,int iHostID, EMdbRepState eState);
    private:
        int ConnectMDB();//����MDB
        int ConnectServer();//�������÷���
        //int StartRepProcess();//����ͬ������
        //int MonitorProcess();//��ؽ��̵�����
        int DealServerCmd(TRepNTCMsg& tCmd);//�����������÷��������
    private:
        char m_sDsn[MAX_NAME_LEN];//Dsn����
        TMdbDatabase *m_pMDB;//���ݿ�����ָ��
        
        TMdbRepConfig *m_pRepConfig;//�����ļ�
        TMdbRepMonitorClient* m_pMonitorClient;//״̬��ؿͻ���    
        TMdbShmRepMgr *m_pShmMgr;//�����ڴ������

        TMdbProcCtrl m_tProcCtrl;
        bool m_bSvrConn; // ���÷�����Ƿ�����
    };

	class TMdbShardBuckupCfgCtrl
	{
	public:
		TMdbShardBuckupCfgCtrl();
		~TMdbShardBuckupCfgCtrl();
		int Init(const char * pszDSN);
		int GetShardBuckupInfo();
	public:
		const char * m_sDsn;
		TMdbRepConfig * m_pRepConfig;
		bool m_bSvrConnFlag;
		TMdbRepMonitorClient * m_pMonitorClient;
		TMdbShmRepMgr * m_pShmMgr;
	};
	
//}
#endif
