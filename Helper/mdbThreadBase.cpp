/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   dgThreadBase.cpp		
*@Description： 
*@Author:		jin.shaohua
*@Date：	    2013/04/03
*@History:
******************************************************************************************/
#include "Helper/mdbThreadBase.h"
#include <iostream>
#include <errno.h>
#include <string.h>
#include <stdio.h>
using namespace std;


#ifndef _WIN32    
#include <unistd.h>
#include <stdlib.h>    
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#else
#include <windows.h>
#endif

//namespace QuickMDB{

    void* TMdbThreadBase::agent(void* p)
    {
    	if(NULL == p)
    	{
    		printf("agent p = NULL...\n");
    		return 0;
    	}
    	TMdbThreadBase* pThread = (TMdbThreadBase*)p;
    	pThread->svc();
		pThread->Clean();
    	return 0;
    }

	void TMdbThreadBase::Clean()
    {
        m_iTID = -1; 
        m_lpVoid = NULL;  
        m_iStack = 0;
    }

    TMdbThreadBase::TMdbThreadBase()
    {
    	m_iTID = -1; 
    	m_lpVoid = NULL;  
    	m_iStack = 0;
    }

    TMdbThreadBase::~TMdbThreadBase()
    {

    }

    /******************************************************************************
    * 函数名称	:  SetThreadInfo()
    * 函数描述	:  设置线程的相关信息    
    * 输入		:  parg 函数参数   
    * 输入		:  stack 函数栈空间大小 
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbThreadBase::SetThreadInfo(LPVOID parg, unsigned int stack)
    {
    	m_lpVoid = parg;
    	m_iStack = stack;
    }


    /******************************************************************************
    * 函数名称	:  Run()
    * 函数描述	:  启动线程，具体动作需要子类重写    
    * 输入		:  无    
    * 输出		:  unsigned int* phandle，在Windows下面返回的句柄，如果在Linux/Unix下面忽略之
    * 返回值	:  成功返回0，如果发生错误，返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbThreadBase::Run(unsigned int* phandle)
    {
    	int hthread;     

#ifdef WIN32
    	hthread = (int)CreateThread(NULL, m_iStack, (LPTHREAD_START_ROUTINE)agent, m_lpVoid, 0, (LPDWORD)&m_iTID);

    	if(hthread != 0 && phandle) 
    	{
    		*phandle = (unsigned int)hthread;
    	}
#else
    	//pthread_t      dwThreadId;
    	pthread_attr_t thread_attr;

    	pthread_attr_init(&thread_attr);
    	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    	pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
    	if(m_iStack > 0)
    		pthread_attr_setstacksize(&thread_attr, m_iStack);

    	if (0 != pthread_create((pthread_t*)&m_iTID, &thread_attr, agent, m_lpVoid)) 
    	{
    		hthread  = 0;
    	} 
    	else 
    	{
    		hthread  = 1;
    	}
    	pthread_attr_destroy(&thread_attr);
#endif

    	if(hthread == 0)
    		return 1;
    	else
    		return 0;    
    }


    /******************************************************************************
    * 函数名称	:  GetTID()
    * 函数描述	:  获取线程的ID    
    * 输入		:  无    
    * 输出		:  无
    * 返回值	:  线程的ID，如果发生错误，返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbThreadBase::GetTID()
    {
    	return m_iTID;
    }


    /******************************************************************************
    * 函数名称	:  KillThread()
    * 函数描述	:  杀死线程    
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  如果成功=0，如果发生错误，返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbThreadBase::KillThread()
    {
    	if(m_iTID == -1)
    		return -1;

    	int iRet = 0;

#ifdef _WIN32
    	iRet = TerminateThread((HANDLE)m_iTID, 0);
#else
    	iRet = pthread_cancel((pthread_t)m_iTID);
#endif

    	return iRet;    
    }
//}    
