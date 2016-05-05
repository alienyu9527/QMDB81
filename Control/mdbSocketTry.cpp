/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbSocketTry.h		
*@Description： 检测网络链接，主要是进行对端的端口探测
*@Author:		li.shugang
*@Date：	    2008年12月15日
*@History:
******************************************************************************************/
#include "Helper/mdbStruct.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbSocketTry.h"

#ifdef WIN32
#include <iostream>
#include <windows.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#endif

//namespace QuickMDB{

    TMdbSocketTry::TMdbSocketTry()
    {

    }

    TMdbSocketTry::~TMdbSocketTry()
    {

    }
        
    /******************************************************************************
    * 函数名称	:  TrySocket()
    * 函数描述	:  尝试对端的某个端口是否可以链接
    * 输入		:  pszIP, 对端的IP 
    * 输入		:  iPort, 对端的Port
    * 输出		:  无
    * 返回值	:  如果可以链接返回true, 否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TMdbSocketTry::TrySocket(const char* pszIP, int iPort)
    {
        TADD_FUNC("TMdbSocketTry::TrySocket(%s, %d) : Start.", pszIP, iPort);	
        
        //将基本名字和地址转换
        struct hostent *hostent = gethostbyname(pszIP);
        	
    	//建立一个TCP套接口
    	long iSocketHandle = socket(AF_INET, SOCK_STREAM, 0);
        if(iSocketHandle == -1) 
        {
    		TADD_ERROR(ERROR_UNKNOWN,"TMdbSocketTry::TrySocket(%s,%d) : Can't create a socket!", 
    		     pszIP, iPort);
    		return false;
        }	
    	
        //初始化结构体，连接到服务器的端口
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(iPort);
        addr.sin_addr   = *((struct in_addr *)hostent->h_addr);
        memset(&(addr.sin_zero), 0, sizeof(addr.sin_zero));

    	int iSec = MAX_TRY_SECONDS;
    	while(iSec >= 0)
    	{
    	    //和服务器建立连接
    	    if(connect(iSocketHandle, (struct sockaddr *)&addr, sizeof(struct sockaddr)) != -1)
    	    {
    	        TADD_FLOW("TMdbSocketTry::TrySocket(%s, %d) : Connect OK!", pszIP, iPort);
#ifdef WIN32
    			closesocket(iSocketHandle);
#else
    	        close(iSocketHandle);
#endif
    			return true;
    	    }
    	    
    	    --iSec;
    	    TMdbDateTime::Sleep(1);
    	}
    	
    	TADD_FLOW("TMdbSocketTry::TrySocket(%s, %d) : Can't connect to peer.", pszIP, iPort);	
    	TADD_FUNC("TMdbSocketTry::TrySocket(%s, %d) : Finish.", pszIP, iPort);	
    	return false;
    }

//}    


