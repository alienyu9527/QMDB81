/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbShm.cpp		
*@Description： 负责管理miniDB的共享内存的控制
*@Author:		li.shugang
*@Date：	    2009年2月3日
*@History:
******************************************************************************************/
#include "Helper/mdbShm.h"

#include "Helper/TThreadLog.h"    //日志


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
* 函数名称  :  IsShmExist
* 函数描述  :  iShmKey - key
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  true-存在 false-不存在
* 作者      :  jin.shaohua
*******************************************************************************/
bool TMdbShm::IsShmExist(SMKey iShmKey, size_t iShmSize,SHAMEM_T & iShmID)
{

#ifdef WIN32
        iShmID = OpenFileMapping(FILE_MAP_ALL_ACCESS,false,(char *) iShmKey);
#else
        //IPC_EXCL代表连原有的(如果原来没有就返回-1)，
        //IPC_CREAT代表新建或者连原有的(如果原来没有就新建，否则就连原有的)
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
* 函数名称	:  Create()
* 函数描述	:  创建共享内存   
* 输入		:  iShmKey, 关键字Key  
* 输入		:  iShmSize, 共享内存大小
* 输出		:  iShmID, 返回的共享内存ID
* 返回值	:  成功返回0，否则返回<0
* 作者		:  li.shugang
*******************************************************************************/

int TMdbShm::Create(SMKey iShmKey, size_t iShmSize, SHAMEM_T & iShmID)
{
#ifdef WIN32
        iShmID = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,iShmSize,(char *)iShmKey);
        if(INITVAl == iShmID)
        {//创建失败
            TADD_ERROR(ERR_OS_CREATE_SHM,"CreateShm failed,[%d]",GetLastError());
            return ERR_OS_CREATE_SHM;
        }
        return 0;
#else
        //首先要排斥性创建，如果失败，则取出排斥属性创建
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
* 函数名称	:  AttachByID()
* 函数描述	:  根据ID连接共享内存   
* 输入		:  iShmID, 共享内存ID  
* 输出		:  pAddr, 返回的共享内存起始地址
* 返回值	:  成功返回0，否则返回<0
* 作者		:  li.shugang
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
* 函数名称	:  AttachByKey()
* 函数描述	:  根据Key连接共享内存   
* 输入		:  iShmKey, 共享内存的Key  
* 输出		:  pAddr, 返回的共享内存起始地址
* 返回值	:  成功返回0，否则返回<0
* 作者		:  li.shugang
*******************************************************************************/
int TMdbShm::AttachByKey(SMKey iShmKey, char* &pAddr)
{
        //获取共享内存标识ID
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
* 函数名称	:  Detach()
* 函数描述	:  断开与共享内存的连接  
* 输入		:  pAddr, 返回的共享内存起始地址 
* 输出		:  无
* 返回值	:  成功返回0，否则返回<0
* 作者		:  li.shugang
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
* 函数名称	:  Destroy()
* 函数描述	:  释放共享内存   
* 输入		:  iShmID, 共享内存ID  
* 输出		:  无
* 返回值	:  成功返回0，否则返回<0
* 作者		:  li.shugang
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
