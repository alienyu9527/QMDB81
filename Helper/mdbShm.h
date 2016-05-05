/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbShm.h		
*@Description�� �������miniDB�Ĺ����ڴ�Ŀ���
*@Author:		li.shugang
*@Date��	    2009��2��3��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_SHARE_MEMORY_H__
#define __MINI_DATABASE_SHARE_MEMORY_H__

#ifdef  _WIN32
    #pragma warning(disable:4312)           //unsigned int ����int ת����HANDLE
    #pragma warning(disable:4311)           //void*ת����long
    #include <windows.h>
#else   //_UNIX
    #include <sys/shm.h>
    #include <stdlib.h>
    #include <sys/ipc.h>
    #include <errno.h>
    #include <signal.h>
#endif  //_WIN32 _UNIX
#include "Helper/mdbErr.h"
#include "Helper/mdbStruct.h"

//namespace QuickMDB{

#define MAX_SHM_COUNTS 11000
class TGlobalShm
{
public:
    TGlobalShm()
    {
        iShmID = INITVAl;
        pAddr  = NULL;
    }
    SHAMEM_T iShmID;
    char* pAddr;
};


class TMdbShm
{   
public:
    /******************************************************************************
    * ��������	:  IsShmExist
    * ��������	:  iShmKey - key
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  true-���� false-������
    * ����		:  jin.shaohua
    *******************************************************************************/
    static bool  SearchExistShm(SMKey iShmKey);
    static bool IsShmExist(SMKey iShmKey, size_t iShmSize,SHAMEM_T & iShmID);
    /******************************************************************************
    * ��������	:  Create()
    * ��������	:  ���������ڴ�   
    * ����		:  iShmKey, �ؼ���Key  
    * ����		:  iShmSize, �����ڴ��С
    * ���		:  iShmID, ���صĹ����ڴ�ID
    * ����ֵ	:  �ɹ�����0/1(�����ڴ�����Ϊ0,���ڷ�����Ϊ1)�����򷵻�<0
    * ����		:  li.shugang
    *******************************************************************************/
    static int Create(SMKey iShmKey, size_t iShmSize, SHAMEM_T & iShmID);
    
    /******************************************************************************
    * ��������	:  AttachByID()
    * ��������	:  ����ID���ӹ����ڴ�   
    * ����		:  iShmID, �����ڴ�ID  
    * ���		:  pAddr, ���صĹ����ڴ���ʼ��ַ
    * ����ֵ	:  �ɹ�����0�����򷵻�<0
    * ����		:  li.shugang
    *******************************************************************************/
    static int AttachByID(SHAMEM_T iShmID, char* &pAddr);
    
    /******************************************************************************
    * ��������	:  AttachByKey()
    * ��������	:  ����Key���ӹ����ڴ�   
    * ����		:  iShmKey, �����ڴ��Key  
    * ���		:  pAddr, ���صĹ����ڴ���ʼ��ַ
    * ����ֵ	:  �ɹ�����0�����򷵻�<0
    * ����		:  li.shugang
    *******************************************************************************/
    static int AttachByKey(SMKey iShmKey, char* &pAddr);

    /******************************************************************************
    * ��������	:  Detach()
    * ��������	:  �Ͽ��빲���ڴ������  
    * ����		:  pAddr, ���صĹ����ڴ���ʼ��ַ 
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�<0
    * ����		:  li.shugang
    *******************************************************************************/
    static int Detach(char* pAddr);

    /******************************************************************************
    * ��������	:  Destroy()
    * ��������	:  �ͷŹ����ڴ�   
    * ����		:  iShmID, �����ڴ�ID  
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�<0
    * ����		:  li.shugang
    *******************************************************************************/
    static int Destroy(SHAMEM_T iShmID);

public:
    static TGlobalShm tGlobalShm[MAX_SHM_COUNTS];
};
//}


#endif //__MINI_DATABASE_SHARE_MEMORY_H__


