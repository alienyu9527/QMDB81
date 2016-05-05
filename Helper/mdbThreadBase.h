/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   dgThreadBase.h		
*@Description�� �̻߳�����
*@Author:		jin.shaohua
*@Date��	    2013/04/03
*@History:
******************************************************************************************/
#ifndef _DATA_GRID_THREAD_BASE_H_
#define _DATA_GRID_THREAD_BASE_H_
#include "Helper/mdbStruct.h"

//namespace QuickMDB{

    class TMdbThreadBase
    {
    public:
        TMdbThreadBase();
        virtual ~TMdbThreadBase(); 
        void SetThreadInfo(LPVOID parg, unsigned int stack=1024*1024);//�����̲߳���
        int  Run(unsigned int* phandle);//�����̣߳����嶯����Ҫ������д    
    	int GetTID();//��ȡ�̵߳�ID    
    	int KillThread();//KillThread()

    protected:
        virtual int svc()=0;//������̶߳���
        static void* agent(void* p); //�м��һ��ת�Ӻ���
        
        int m_iTID; 
        LPVOID m_lpVoid;
        unsigned int m_iStack;
    };

//}
#endif


