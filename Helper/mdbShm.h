/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbShm.h		
*@Description： 负责管理miniDB的共享内存的控制
*@Author:		li.shugang
*@Date：	    2009年2月3日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_SHARE_MEMORY_H__
#define __MINI_DATABASE_SHARE_MEMORY_H__

#ifdef  _WIN32
    #pragma warning(disable:4312)           //unsigned int 或者int 转换到HANDLE
    #pragma warning(disable:4311)           //void*转换到long
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
    * 函数名称	:  IsShmExist
    * 函数描述	:  iShmKey - key
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  true-存在 false-不存在
    * 作者		:  jin.shaohua
    *******************************************************************************/
    static bool  SearchExistShm(SMKey iShmKey);
    static bool IsShmExist(SMKey iShmKey, size_t iShmSize,SHAMEM_T & iShmID);
    /******************************************************************************
    * 函数名称	:  Create()
    * 函数描述	:  创建共享内存   
    * 输入		:  iShmKey, 关键字Key  
    * 输入		:  iShmSize, 共享内存大小
    * 输出		:  iShmID, 返回的共享内存ID
    * 返回值	:  成功返回0/1(不存在创建则为0,存在返回则为1)，否则返回<0
    * 作者		:  li.shugang
    *******************************************************************************/
    static int Create(SMKey iShmKey, size_t iShmSize, SHAMEM_T & iShmID);
    
    /******************************************************************************
    * 函数名称	:  AttachByID()
    * 函数描述	:  根据ID连接共享内存   
    * 输入		:  iShmID, 共享内存ID  
    * 输出		:  pAddr, 返回的共享内存起始地址
    * 返回值	:  成功返回0，否则返回<0
    * 作者		:  li.shugang
    *******************************************************************************/
    static int AttachByID(SHAMEM_T iShmID, char* &pAddr);
    
    /******************************************************************************
    * 函数名称	:  AttachByKey()
    * 函数描述	:  根据Key连接共享内存   
    * 输入		:  iShmKey, 共享内存的Key  
    * 输出		:  pAddr, 返回的共享内存起始地址
    * 返回值	:  成功返回0，否则返回<0
    * 作者		:  li.shugang
    *******************************************************************************/
    static int AttachByKey(SMKey iShmKey, char* &pAddr);

    /******************************************************************************
    * 函数名称	:  Detach()
    * 函数描述	:  断开与共享内存的连接  
    * 输入		:  pAddr, 返回的共享内存起始地址 
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回<0
    * 作者		:  li.shugang
    *******************************************************************************/
    static int Detach(char* pAddr);

    /******************************************************************************
    * 函数名称	:  Destroy()
    * 函数描述	:  释放共享内存   
    * 输入		:  iShmID, 共享内存ID  
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回<0
    * 作者		:  li.shugang
    *******************************************************************************/
    static int Destroy(SHAMEM_T iShmID);

public:
    static TGlobalShm tGlobalShm[MAX_SHM_COUNTS];
};
//}


#endif //__MINI_DATABASE_SHARE_MEMORY_H__


