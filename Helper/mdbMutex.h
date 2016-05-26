/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbMutex.h		
*@Description�� �ڴ����ݿ�����������
*@Author:		li.shugang
*@Date��	    2008��11��30��
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_MUTX_CTROL_H__
#define __ZX_MINI_DATABASE_MUTX_CTROL_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"
#include "Helper/TMutex.h"

//namespace QuickMDB{

    //�����ͣ���ʼ������ʱ����
    enum E_MUTEX_TYPE
    {
    	MUTEX_TYPE_PAGE  = 1, //��ͨҳ��
    	MUTEX_TYPE_VARCHAR_PAGE = 2, //varcharҳ��
    	MUTEX_TYPE_ROW = 3 //����
    };

    class TMdbMutex
    {
    public:
        TMdbMutex()
        {
            m_bInitFlag = false;
            m_pMutex    = NULL;
			iMutexCount = 0;
        }
        
        
        /******************************************************************************
        * ��������	:  Init()
        * ��������	:  ��ʼ������ַ    
        * ����		:  pszDSN, ��������������DSN 
        * ���		:  pDSNOut, ����Ӧ��DSNָ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Init(const char* pszDSN, TMdbDSN* &pDSNOut,E_MUTEX_TYPE eMutexType);
        
        /******************************************************************************
        * ��������	:  Lock()
        * ��������	:  ��ĳ��λ�ü���    
        * ����		:  �� 
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        int Lock(int iPos);
        
        /******************************************************************************
        * ��������	:  UnLock()
        * ��������	:  ��ĳ��λ�ý���    
        * ����		:  �� 
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void UnLock(int iPos);
        
    private:
        bool m_bInitFlag;    
        TMutex *m_pMutex;
        int m_iPID;
        //int m_iTID;
    	unsigned long int m_iTID;
        TMdbDSN* m_pDSN;
		int iMutexCount;
    };
//}


#endif //__ZX_MINI_DATABASE_MUTX_CTROL_H__


