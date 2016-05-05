/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbSocketTry.h		
*@Description： 检测网络链接，主要是进行对端的端口探测
*@Author:		li.shugang
*@Date：	    2008年12月15日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_SOCKET_TRY_H__
#define __MINI_DATABASE_SOCKET_TRY_H__



//namespace QuickMDB{


    class TMdbSocketTry
    {
    public:
        TMdbSocketTry();
        ~TMdbSocketTry();
        
        /******************************************************************************
        * 函数名称	:  TrySocket()
        * 函数描述	:  尝试对端的某个端口是否可以链接
        * 输入		:  pszIP, 对端的IP 
        * 输入		:  iPort, 对端的Port
        * 输出		:  无
        * 返回值	:  如果可以链接返回true, 否则返回false
        * 作者		:  li.shugang
        *******************************************************************************/
        static bool TrySocket(const char* pszIP, int iPort);
        
    };
//}

#endif //__MINI_DATABASE_SOCKET_TRY_H__

