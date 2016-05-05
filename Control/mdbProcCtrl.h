/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbProcCtrl.h		
*@Description�� �ڴ����ݿ�Ľ��̹������
*@Author:		li.shugang
*@Date��	    2008��12��05��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_PROCESS_CONTRL_H__
#define __MINI_DATABASE_PROCESS_CONTRL_H__

#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbJson.h"

//namespace QuickMDB{

    

    class TMdbProcCtrl
    {
    public:
        TMdbProcCtrl();
        ~TMdbProcCtrl();

        /******************************************************************************
        * ��������	:  Init()
        * ��������	:  ��ʼ����Attach�����ڴ棬��ս�����Ϣ  
        * ����		:  pszDSN, ��������������DSN  
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Init(const char* pszDSN);


        /******************************************************************************
        * ��������	:  Restart()
        * ��������	:  ��������ĳ������  
        * ����		:  pszProcName, ������
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Restart(const char* pszProcName,bool bSkipWarnLog = false);

        /******************************************************************************
        * ��������	:  StopAll()
        * ��������	:  ֹͣ���н���  
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int StopAll();
        bool IsDBStop();
        void ClearProcInfo();
        TMdbProc*   RegProc(const char * sProcName,const bool bIsMonitor = true);
        int UpdateProcHeart(int iWaitSec);//����ĳ��������
        int ScanAndClear();//��Ⲣ���������Ϣ
        bool IsCurProcStop();//��⵱ǰ�����Ƿ���Ҫֹͣ
        bool IsMonitorStart();//��ؽ����Ƿ��Ѿ�����
        bool IsMdbSysProcess(const char * sProcName);//�ж��ǲ���MDBϵͳ����
        int ResetLogLevel();//���ý�����־����
        int Serialize(rapidjson::PrettyWriter<TMdbStringStream> & writer);//���л�
        bool IsMdbSysProcExist(const char * sDsn);//�Ƿ���ϵͳ���̴���
        bool IsProcExist(TMdbProc * pProc,const char * sCurTime);//�ý����Ƿ����
        int StopProcess(const int iPID,const int iHeartBeatFatal);//ͣ��ָ������
        bool IsLongHeartBeatProcess(TMdbProc *pProc);//�ж��Ƿ�Ϊ��������ؽ���
        char GetProcState();//��ȡ����״̬
    private:
        TMdbShmDSN* m_pShmDSN;
        TMdbConfig *m_pConfig;
        TMdbDSN    *m_pDsn;    
        TMdbProc * m_pCurProc;//��¼��ǰʹ�õ�
    };
//}

#endif //__MINI_DATABASE_PROCESS_CONTRL_H__

