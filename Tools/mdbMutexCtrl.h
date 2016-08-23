/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbMutexCtrl.h		
*@Description�� �ڴ����ݿ�������Ƴ���
*@Author:		jiang.mingjun
*@Date��	    2010��05��05��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE__MUTEX_CTROL_H__
#define __MINI_DATABASE__MUTEX_CTROL_H__

#include "Helper/mdbConfig.h"

//namespace QuickMDB{


    class TMdbMutexCtrl
    {
    public:
        TMdbMutexCtrl();
        ~TMdbMutexCtrl();
    public:
        int Init(const char* pszDsn);     //��ʼ��
        int ReNewAllMutex();   //����ĳ���ڴ����ݿ�    
        int RenewDsnMutex();
        int RenewSeqMutex(const char * sSeqname = NULL);
        int RenewDataMutex();
        int RenewTableMutex(const char * sTablename = NULL);
        int RenewPageMutex();
        int RenewTablespaceMutex(const char * sTSname = NULL);
        #if 0
        int RenewOraMutex();
        int RenewRepMutex();
        int RenewCaptureMutex();
        #endif
        
        int RenewIndexMutex();
        int RenewSyncAreaMutex(int iType);
		int RenewShardBakBufAreaMutex(int iType);
    private:
        int RenewOneMutex(TMutex & tMutex);
        TMdbShmDSN * m_pShmDsn;
    };

//}

#endif //__MINI_DATABASE_CTROL_H__


