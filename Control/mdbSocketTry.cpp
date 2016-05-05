/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbSocketTry.h		
*@Description�� ����������ӣ���Ҫ�ǽ��жԶ˵Ķ˿�̽��
*@Author:		li.shugang
*@Date��	    2008��12��15��
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
    * ��������	:  TrySocket()
    * ��������	:  ���ԶԶ˵�ĳ���˿��Ƿ��������
    * ����		:  pszIP, �Զ˵�IP 
    * ����		:  iPort, �Զ˵�Port
    * ���		:  ��
    * ����ֵ	:  ����������ӷ���true, ���򷵻�false
    * ����		:  li.shugang
    *******************************************************************************/
    bool TMdbSocketTry::TrySocket(const char* pszIP, int iPort)
    {
        TADD_FUNC("TMdbSocketTry::TrySocket(%s, %d) : Start.", pszIP, iPort);	
        
        //���������ֺ͵�ַת��
        struct hostent *hostent = gethostbyname(pszIP);
        	
    	//����һ��TCP�׽ӿ�
    	long iSocketHandle = socket(AF_INET, SOCK_STREAM, 0);
        if(iSocketHandle == -1) 
        {
    		TADD_ERROR(ERROR_UNKNOWN,"TMdbSocketTry::TrySocket(%s,%d) : Can't create a socket!", 
    		     pszIP, iPort);
    		return false;
        }	
    	
        //��ʼ���ṹ�壬���ӵ��������Ķ˿�
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(iPort);
        addr.sin_addr   = *((struct in_addr *)hostent->h_addr);
        memset(&(addr.sin_zero), 0, sizeof(addr.sin_zero));

    	int iSec = MAX_TRY_SECONDS;
    	while(iSec >= 0)
    	{
    	    //�ͷ�������������
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


