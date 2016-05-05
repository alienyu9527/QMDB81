/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbShm.cpp		
*@Description�� �������miniDB�Ĺ����ڴ�Ŀ���
*@Author:		li.shugang
*@Date��	    2009��2��3��
*@History:
******************************************************************************************/
#include "Helper/mdbShm.h"

#include "Helper/TThreadLog.h"    //��־


//namespace QuickMDB{

#ifdef WIN32
#pragma warning(disable:4006)
#endif

TGlobalShm TMdbShm::tGlobalShm[MAX_SHM_COUNTS];

bool  TMdbShm::SearchExistShm(SMKey iShmKey)
{
	char Cmd[256];
	char buf[256];
	memset(Cmd,0,sizeof(Cmd));
	memset(buf,0,sizeof(buf));
#if defined(OS_SUN) || defined(OS_HP) || defined(OS_IBM)
	SAFESTRCPY(Cmd,sizeof(Cmd),"ipcs -m|grep -i '0x'|awk '{print $3}'");
#else
	SAFESTRCPY(Cmd,sizeof(Cmd),"ipcs -m|grep -i '0x'|awk '{print $1}'");
#endif
	FILE* ptrPipe = NULL;
	char* ptrStr = NULL;
	ptrPipe = popen(Cmd, "r");
	long long keyValue = 0;

	if (ptrPipe)
	{
		while (fgets(buf, sizeof(buf), ptrPipe ) != NULL)
		{
			ptrStr = strstr(buf,"\n");
			if(ptrStr != NULL)
				ptrStr = '\0';
			
			sscanf(buf, "%x", &keyValue);
			
			if(keyValue == iShmKey)
			{
				pclose(ptrPipe);
				return true;
			}
		}

		pclose(ptrPipe);
	}
	return false;
	

}
/******************************************************************************
* ��������  :  IsShmExist
* ��������  :  iShmKey - key
* ����      :  
* ����      :  
* ���      :  
* ����ֵ    :  true-���� false-������
* ����      :  jin.shaohua
*******************************************************************************/
bool TMdbShm::IsShmExist(SMKey iShmKey, size_t iShmSize,SHAMEM_T & iShmID)
{

#ifdef WIN32
        iShmID = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,(char *) iShmKey);
#else
        //IPC_EXCL������ԭ�е�(���ԭ��û�оͷ���-1)��
        //IPC_CREAT�����½�������ԭ�е�(���ԭ��û�о��½����������ԭ�е�)
        iShmID = shmget(iShmKey, iShmSize, 0666 | IPC_EXCL);
#endif
        if(iShmID  != INITVAl)
        {
            return true;
        }
        else
        {
           // return false;
           return SearchExistShm(iShmKey);
        }

	
	
}

/******************************************************************************
* ��������	:  Create()
* ��������	:  ���������ڴ�   
* ����		:  iShmKey, �ؼ���Key  
* ����		:  iShmSize, �����ڴ��С
* ���		:  iShmID, ���صĹ����ڴ�ID
* ����ֵ	:  �ɹ�����0�����򷵻�<0
* ����		:  li.shugang
*******************************************************************************/

int TMdbShm::Create(SMKey iShmKey, size_t iShmSize, SHAMEM_T & iShmID)
{
#ifdef WIN32
        iShmID = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,iShmSize,(char *)iShmKey);
        if(INITVAl == iShmID)
        {//����ʧ��
            TADD_ERROR(ERR_OS_CREATE_SHM,"CreateShm failed,[%d]",GetLastError());
            return ERR_OS_CREATE_SHM;
        }
        return 0;
#else
        //����Ҫ�ų��Դ��������ʧ�ܣ���ȡ���ų����Դ���
        iShmID = shmget(iShmKey, iShmSize, 0666|IPC_CREAT|IPC_EXCL);
        if(iShmID == -1)
        {
            iShmID = shmget(iShmKey, iShmSize, 0666|IPC_CREAT);  
            if(iShmID == -1)  
            {
                TADD_ERROR(ERR_OS_CREATE_SHM,"CreateShm failed, iShmKey=%lld, iShmSize=%ld,errno=%d,errmsg=%s .",iShmKey, iShmSize,errno,strerror(errno));
                return ERR_OS_CREATE_SHM;    
            }
        }
        else
        {
            return 0;    
        } 
        return 0;
#endif
}


/******************************************************************************
* ��������	:  AttachByID()
* ��������	:  ����ID���ӹ����ڴ�   
* ����		:  iShmID, �����ڴ�ID  
* ���		:  pAddr, ���صĹ����ڴ���ʼ��ַ
* ����ֵ	:  �ɹ�����0�����򷵻�<0
* ����		:  li.shugang
*******************************************************************************/
int TMdbShm::AttachByID(SHAMEM_T iShmID, char* &pAddr)
{   

    for(int i=0; i<MAX_SHM_COUNTS; ++i)
    {
        if(tGlobalShm[i].iShmID == iShmID)
        {
            pAddr = tGlobalShm[i].pAddr;
            return 0;
        }
    }
#ifdef WIN32
        pAddr = (char*)MapViewOfFile(iShmID, FILE_MAP_ALL_ACCESS,0,0,0);
#else
        pAddr = (char*)shmat(iShmID, 0, 0);
#endif
    if(pAddr == (char*)-1)
    {
        TADD_ERROR(ERR_OS_ATTACH_SHM,"errorno=%d, error-msg=%s\n,iShmID=%d", errno, strerror(errno),iShmID); 
        pAddr = NULL;
        return ERR_OS_ATTACH_SHM;
    }
    for(int i=0; i<MAX_SHM_COUNTS; ++i)
    {
        if(tGlobalShm[i].iShmID == -1)
        {
            tGlobalShm[i].pAddr = pAddr;
            tGlobalShm[i].iShmID = iShmID;
            break;
        }
    }
    return 0;
}

    
/******************************************************************************
* ��������	:  AttachByKey()
* ��������	:  ����Key���ӹ����ڴ�   
* ����		:  iShmKey, �����ڴ��Key  
* ���		:  pAddr, ���صĹ����ڴ���ʼ��ַ
* ����ֵ	:  �ɹ�����0�����򷵻�<0
* ����		:  li.shugang
*******************************************************************************/
int TMdbShm::AttachByKey(SMKey iShmKey, char* &pAddr)
{
        //��ȡ�����ڴ��ʶID
        SHAMEM_T iShmID = INITVAl;
#ifdef WIN32
        iShmID = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,(char *) iShmKey);
#else
        iShmID = shmget(iShmKey, 0, 0666);
#endif

    if(iShmID < 0)
    {   
        return ERR_OS_ATTACH_SHM;
    }
    if(AttachByID(iShmID,pAddr) < 0)
    {
        return ERR_OS_ATTACH_SHM;
    }
    return 0;
}


/******************************************************************************
* ��������	:  Detach()
* ��������	:  �Ͽ��빲���ڴ������  
* ����		:  pAddr, ���صĹ����ڴ���ʼ��ַ 
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�<0
* ����		:  li.shugang
*******************************************************************************/
int TMdbShm::Detach(char* pAddr)
{
#ifdef WIN32
    if(false == UnmapViewOfFile(pAddr))
    {
        return ERR_OS_DETACH_SHM;
    }
#else
    int iRet = shmdt(pAddr);
    if(iRet != 0)
    {
    	TADD_ERROR(ERR_OS_DETACH_SHM,"errorno=%d, error-msg=%s\n,pAddr=%p", errno, strerror(errno),pAddr); 
        return ERR_OS_DETACH_SHM;
    }
#endif
    return 0;
}


/******************************************************************************
* ��������	:  Destroy()
* ��������	:  �ͷŹ����ڴ�   
* ����		:  iShmID, �����ڴ�ID  
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�<0
* ����		:  li.shugang
*******************************************************************************/
int TMdbShm::Destroy(SHAMEM_T iShmID)
{
#ifdef WIN32
    if(false == CloseHandle(iShmID))
    {
        return ERR_OS_DESTROY_SHME;
    }
#else
    int iRet = shmctl(iShmID, IPC_RMID, 0);
    if(iRet != 0)
    {
    	TADD_ERROR(ERR_OS_DESTROY_SHME,"errorno=%d, error-msg=%s\n,iShmID=%d",errno, strerror(errno),iShmID); 
        return ERR_OS_DESTROY_SHME;
    }
#endif
    return 0;
}


//}
