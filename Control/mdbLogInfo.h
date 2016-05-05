/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbLogInfo.h		
*@Description�� ���������ڴ����ݿ�ĸ�����־��Ϣ
*@Author:		li.shugang
*@Date��	    2009��04��23��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_LOG_INFORMATION_H__
#define __MINI_DATABASE_LOG_INFORMATION_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

    class TMdbLogInfo
    {
    public:
        TMdbLogInfo();
        ~TMdbLogInfo();
        
        /******************************************************************************
        * ��������	:  Connect()
        * ��������	:  ����ĳ��DSN�����ǲ��ڹ�����ע���κ���Ϣ    
        * ����		:  pszDSN, ��������������DSN 
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Connect(const char* pszDSN);   
        

        /******************************************************************************
        * ��������	:  SetLocalLink()
        * ��������	:  ���ñ���������־����    
        * ����		:  iPid������ID
        * ����		:  iTid���߳�ID
        * ����		:  iLogLevel, ��־����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
        * ����		:  li.shugang
        *******************************************************************************/
        int SetLocalLink(int iPid, int iTid, int iLogLevel);
        int SetLocalLink(int iPid, int iLogLevel);
        int SetLocalLink(TMdbDSN  *pDsn, int iPid, int iLogLevel);
        int SetLocalLink(TMdbDSN  *pDsn, int iPid, int iTid, int iLogLevel);
        /******************************************************************************
        * ��������	:  SetRemoteLink()
        * ��������	:  ����Զ��������־����    
        * ����		:  pszIP, Զ��IP
        * ����		:  iLogLevel, ��־����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
        * ����		:  li.shugang
        *******************************************************************************/
        int SetRemoteLink(const char* pszIP, int iLogLevel);
        int SetRemoteLink(TMdbDSN  *pDsn,int pid, int iLogLevel);
        int SetRemoteLink(TMdbDSN  *pDsn,int pid,int tid, int iLogLevel);
        /******************************************************************************
        * ��������	:  SetMonitor()
        * ��������	:  ���ü�ؽ��̵���־����    
        * ����		:  iLogLevel, ��־����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
        * ����		:  li.shugang
        *******************************************************************************/
        int SetMonitor(int iLogLevel);
        

        /******************************************************************************
        * ��������	:  SetProc()
        * ��������	:  ���ý�����־��Ϣ,���ݽ��̺�
        * ����		:  iPid, ����ID
        * ����		:  iLogLevel, ��־����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
        * ����		:  li.shugang
        *******************************************************************************/
        int SetProc(int iPid, int iLogLevel);
    	int SetProc(TMdbDSN *pDsn, char *processName, int iLogLevel);
    	int SetProc(TMdbDSN  *pDsn, int ipid, int iLogLevel);
    	/******************************************************************************
        * ��������	:  SetProc()
        * ��������	:  ���ý�����־��Ϣ�����ݽ�������֧��ģ��ƥ�� 
        * ����		:  pProcessName, ������
        * ����		:  iLogLevel, ��־����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ��ش�����
        * ����		:  zhang.lin
        *******************************************************************************/
    	int SetProc(const char *pProcessName, int iLogLevel);
    	
    private:
        
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn;     
    };
//}

#endif //__MINI_DATABASE_LOG_INFORMATION_H__



