/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepServerCtrl.h
*@Description�� ����ͬ�����ݽ��գ������ݼ���ʱͬ�����ݵķ���
*@Author:		jiang.lili
*@Date��	    2014/03/20
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
    * @brief ���ձ���ͬ��������
    * 
    */
    class TRepServerDataRcv:public TMdbNtcBaseObject
    {
    public:
        TRepServerDataRcv();
        ~TRepServerDataRcv();
    public:
        int Init(const char* sDsn, const char* strIP, int iPort);//��ʼ��
        int DealRcvData(const char* sDataBuf, int iBufLen);//������յ�������
        bool IsSame(const char* strIP, int iPort);//�Ƿ�����ͬ�ı���
    private:
        int CombineData(const char* sDataBuf, int iBufLen);
        int GetRecdLen();
        int CheckDataCompletion();
        void SaveRecord();
       
    private:
        /*QuickMDB::*/TMdbNtcString m_strIP;//����IP
        int m_iPort;//�����˿ں�
        char m_sCurTime[MAX_TIME_LEN]; // ��ǰʱ��
        char m_sLinkTime[MAX_TIME_LEN]; //  ����ʱ��
        bool m_bCheckTime; // �Ƿ�У��ʱ���       
        int m_iMsgBufLen;
        char* m_psMsgBuf;
        char m_sLeftBuf[MAX_REP_SEND_BUF_LEN];//ʣ��buf
        char m_sOneRecord[MAX_VALUE_LEN];//һ����¼

        int m_iDealDataLen;//�Ѿ������Msg����
        int m_iLeftDataLen;//ʣ���Msg����
        int m_iTotalDataLen;//���ݵ��ܳ���

        TRepFlushDAOCtrl * m_ptFlushDaoCtrl;
    };

    /**
    * @brief �򱸻���������ʱ�ļ�������
    * 
    */
    class TRepServerDataSend
    {
    public:
        TRepServerDataSend();
        ~TRepServerDataSend();
    public:
        int Init(const char* sDsn);
        int SendData(int iHostID, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo);//��������������
        int SendData(const char* sTableName, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo);//��������������
        int CleanRepFile(int iHostID=MDB_REP_EMPTY_HOST_ID);//ɾ����Ӧ������ͬ���ļ�
    private:
        int DealOneTable(const char* sTableName, const char* sRoutinglist);
        int SendBufData(const char* sBuf, int iLength);//���ͻ������е�����
        void GetOneRecord();//����һ����¼
        
    private:
        TMdbDatabase *m_pDataBase;
        TMdbQuery *m_pCurQuery;//��ǰ������ָ��
        TMdbConfig* m_pConfig;
        TMdbRepConfig *m_pRepConfig;
        
       TRepLoadDao *m_pLoadDao;//��ȡ����
       //std::string m_strRoutingList;//��Ҫ���ص�·���б�
       //int m_iHostID;//��Ӧ������ID
       TMdbPeerInfo* m_pPeerInfo;
       char m_sSendBuf[MAX_REP_SEND_BUF_LEN];//���ͻ�����
       char m_sOneRecord[MAX_VALUE_LEN];//һ����¼
    };

    /**
    * @brief mdbRepServer���̹�����
    * 
    */
    class TRepServer
    {
    public:
        TRepServer();
        ~TRepServer();
        
    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(const char *pszDSN); //��ʼ��
        int Start();//����
    private:
        TMdbRepConfig *m_pRepConfig;//�����ļ�
        TMdbRepDataServer *m_pRepDataServer;//���������
        TMdbShmRepMgr *m_pShmMgr;//�����ڴ������
        const TMdbShmRep *m_pShmRep;//�����ڴ�

        TMdbProcCtrl m_tProcCtrl;
        
    };
//}


#endif //__ZX_MINI_DATABASE_SOCKET_REPLICATION_SERVER_H__

