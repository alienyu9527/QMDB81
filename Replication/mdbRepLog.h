/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepLog.h		
*@Description: �������ڴ�����ͬ�����ݰ�·�ɺ�������ص���ͬ���ļ���ͬһ��·�ɶ�Ӧ�������ʱ������ض��
*@Author:		jiang.lili
*@Date��	    2014/05/4
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
    * @brief ͬ���ļ���Ϣ�ṹ��
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
    * @brief ͬ������д�ļ���
    * 
    */
    class TRoutingRepLog
    {
    public:
        TRoutingRepLog();
        ~TRoutingRepLog();
    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	: ��ʼ�������ӹ����ڴ棬��ȡ�����ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(const char* sDsn, TMdbOnlineRepQueue & mdbOnlineRepQueueCtrl, int iHostID);

        int Log(bool bEmpty = false);	

    private:
        /******************************************************************************
        * ��������	:  Write
        * ��������	:  ��ָ��·������д���Ӧ���ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Write(int iRoutingID, const char* sDataBuf,  int iBufLen);
        /******************************************************************************
        * ��������	:  CheckAndBackup
        * ��������	: ��ʼ�������ӹ����ڴ棬��ȡ�����ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/      
        int CheckAndBackup();     

        void WriteToFile(TRoutingRepFile* pRepFile);

		/******************************************************************************
		* ��������	:  CreateRepFile
		* ��������	: ����ͬ���ļ�
		* ����		:  
		* ���		:  
		* ����ֵ	:  0 - �ɹ�!0 -ʧ��
		* ����		:  jiang.lili
		*******************************************************************************/
		int CreateRepFile(TRoutingRepFile *ptRepFile, int iHostID);

        /******************************************************************************
        * ��������	:  Backup
        * ��������	: ����ͬ���ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Backup(TRoutingRepFile *pRepFile);
                
        /******************************************************************************
        * ��������	:  CheckWriteToFile
        * ��������	:  ���ݻ���д�ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int CheckWriteToFile(TRoutingRepFile*ptRepFile, const char* sDataBuf, int iBufLen);//����д�ļ�

    private:
        TMdbRepConfig *m_pRepConfig;//�����ļ�
        TMdbShmRepMgr *m_pShmMgr;//�����ڴ������
        const TMdbShmRep *m_pShmRep;//�����ڴ�

        TMdbProcCtrl m_tProcCtrl;
        TMdbOnlineRepQueue *m_pOnlineRepQueueCtrl;//�ڴ滺�������
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn; 
        TMdbRepRecdDecode* m_pRecdParser;
        TMdbRedoLogParser m_tParser;
        int m_iMaxFileSize;//�ļ�����С
        char m_sLogPath[MAX_PATH_NAME_LEN];
        long m_iRecord;//�Ѵ���ļ�¼��������������10000���Ժ󣬴��㿪ʼ���¼���
        int  m_iCheckCounts;
        int m_iLen;//�洢һ����¼����
        char* m_spzRecord;//�洢һ����¼
        int m_iRoutID;//��¼��Ӧ��·��ID
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
        * ��������	:  Init
        * ��������	: ��ʼ�������ӹ����ڴ棬��ȡ�����ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
		int Init(const char* sDsn, TMdbQueue & mdbQueueCtrl, TMdbOnlineRepQueue * mdbOnlineRepQueueCtrl);
        int Dispatch(bool bEmpty = false);	

    private:
        TMdbRepConfig *m_pRepConfig;//�����ļ�
        TMdbShmRepMgr *m_pShmMgr;//�����ڴ������
        const TMdbShmRep *m_pShmRep;//�����ڴ�
        TMdbQueue *m_pQueueCtrl;//�ڴ滺�������
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn; 
        TMdbRepRecdDecode* m_pRecdParser;
        TMdbRedoLogParser m_tParser;
        long m_iRecord;//�Ѵ���ļ�¼��������������10000���Ժ󣬴��㿪ʼ���¼���
        int m_iLen;//�洢һ����¼����
        char* m_spzRecord;//�洢һ����¼
        int m_iRoutID;//��¼��Ӧ��·��ID

		TMdbOnlineRepQueue * m_pOnlineRepQueueCtrl[MAX_ALL_REP_HOST_COUNT];
    };

//}

#endif
