/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   dgThreadBase.cpp		
*@Description�� 
*@Author:		jin.shaohua
*@Date��	    2013/04/03
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
    * ��������	:  SetThreadInfo()
    * ��������	:  �����̵߳������Ϣ    
    * ����		:  parg ��������   
    * ����		:  stack ����ջ�ռ��С 
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbThreadBase::SetThreadInfo(LPVOID parg, unsigned int stack)
    {
    	m_lpVoid = parg;
    	m_iStack = stack;
    }


    /******************************************************************************
    * ��������	:  Run()
    * ��������	:  �����̣߳����嶯����Ҫ������д    
    * ����		:  ��    
    * ���		:  unsigned int* phandle����Windows���淵�صľ���������Linux/Unix�������֮
    * ����ֵ	:  �ɹ�����0������������󣬷���-1
    * ����		:  li.shugang
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
    * ��������	:  GetTID()
    * ��������	:  ��ȡ�̵߳�ID    
    * ����		:  ��    
    * ���		:  ��
    * ����ֵ	:  �̵߳�ID������������󣬷���-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbThreadBase::GetTID()
    {
    	return m_iTID;
    }


    /******************************************************************************
    * ��������	:  KillThread()
    * ��������	:  ɱ���߳�    
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ����ɹ�=0������������󣬷���-1
    * ����		:  li.shugang
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
