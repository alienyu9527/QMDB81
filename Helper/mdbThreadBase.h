/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   dgThreadBase.h		
*@Description： 线程基础类
*@Author:		jin.shaohua
*@Date：	    2013/04/03
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
        void SetThreadInfo(LPVOID parg, unsigned int stack=1024*1024);//设置线程参数
        int  Run(unsigned int* phandle);//启动线程，具体动作需要子类重写    
    	int GetTID();//获取线程的ID    
    	int KillThread();//KillThread()

    protected:
        virtual int svc()=0;//子类的线程动作
        static void* agent(void* p); //中间的一个转接函数
        
        int m_iTID; 
        LPVOID m_lpVoid;
        unsigned int m_iStack;
    };

//}
#endif


