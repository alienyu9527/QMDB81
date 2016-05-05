/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbPageCtrl.h		
*@Description�� �������ĳ��ҳ�Ŀռ���Ϣ����ȡһ������ҳ������ҳ
*@Author:		li.shugang
*@Date��	    2008��11��30��
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_PAGE_CTROL_H__
#define __ZX_MINI_DATABASE_PAGE_CTROL_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbMutex.h"
#include "Helper/mdbDictionary.h"

//namespace QuickMDB{
	//ҳ����,������ʱʹ��
    /*enum E_PAGE_TYPE
    {
    	PAGE_TYPE_NO_VARCHAR  = 0, //��varcharҳ
    	PAGE_TYPE_VARCHAR_16 = 1, //varchar16λҳ
    	PAGE_TYPE_VARCHAR_32 = 2, //varchar32λҳ
    	PAGE_TYPE_VARCHAR_64 = 3, //varchar64λҳ
    	PAGE_TYPE_VARCHAR_128 = 4, //varchar128λҳ
    	PAGE_TYPE_VARCHAR_256 = 5, //varchar256λҳ
    	PAGE_TYPE_VARCHAR_512 = 6, //varchar512λҳ
    	PAGE_TYPE_VARCHAR_1024 = 7, //varchar1024λҳ
    	PAGE_TYPE_VARCHAR_2048 = 8, //varchar2048λҳ
    	PAGE_TYPE_VARCHAR_4096 = 9, //varchar4096λҳ
    	PAGE_TYPE_VARCHAR_8192 = 10 //varchar8192λҳ
    };*/

    class TMdbPageCtrl
    {
    public:
        TMdbPageCtrl();
        ~TMdbPageCtrl();
        
        /******************************************************************************
        * ��������	:  SetDSN()
        * ��������	:  ɾ��ҳ��һ������    
        * ����		:  iPos, ���ݵĽڵ�λ��  
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����û���ҵ������򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int SetDSN(const char* pszDSN, E_MUTEX_TYPE eMutexType = MUTEX_TYPE_PAGE);
				
        /******************************************************************************
        * ��������	:  Attach()
        * ��������	:  ����һ��ҳ    
        * ����		:  pAddr, ҳ���׵�ַ 
        * ����		:  bReadLock, �Ƿ��ж���
        * ����		:  bWriteLock, �Ƿ���д�� 
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int Attach(char* pAddr, bool bReadLock, bool bWriteLock); 

        /******************************************************************************
        * ��������	:  InsertData()
        * ��������	:  ��ҳ�в���һ������    
        * ����		:  pData, ���ݵ��׵�ַ  
        * ����		:  iSize, ���ݵĴ�С 
        * ���		:  rowID, ���ص�ROWID
        * ����ֵ	:  �ɹ�����0�����û�пռ��򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int InsertData(unsigned char* pData, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn=true);
	 //�˺����ڲ���Ҷ����ҳ���ں������ֶ���	
	 int InsertData_NoMutex(unsigned char* pData, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn=true);
	 
        /******************************************************************************
        * ��������	:  DeleteData()
        * ��������	:  ɾ��ҳ��һ������    
        * ����		:  iPos, ���ݵĽڵ�λ��  
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����û���ҵ������򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        int DeleteData(int iPos,bool UpdateLsn=true);
	 int DeleteData_NoMutex(int iPos,bool UpdateLsn=true);

        //����
        int RLock();
        int WLock();
        
        //����
        void UnRLock();
        void UnWLock();
        void UpdatePageLSN(){m_pPage->m_iPageLSN = m_pDSN->GetLSN();}//����pageLSN
    private:
        bool m_bReadLock, m_bWriteLock;
        TMdbPage *m_pPage;  //ҳ��ͷ
        char* m_pAddr;      
        TMdbMutex m_tMutex;
        TMdbDSN* m_pDSN;
    };

//}
#endif //__ZX_MINI_DATABASE_PAGE_CTROL_H__

