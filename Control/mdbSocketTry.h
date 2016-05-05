/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbSocketTry.h		
*@Description�� ����������ӣ���Ҫ�ǽ��жԶ˵Ķ˿�̽��
*@Author:		li.shugang
*@Date��	    2008��12��15��
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
        * ��������	:  TrySocket()
        * ��������	:  ���ԶԶ˵�ĳ���˿��Ƿ��������
        * ����		:  pszIP, �Զ˵�IP 
        * ����		:  iPort, �Զ˵�Port
        * ���		:  ��
        * ����ֵ	:  ����������ӷ���true, ���򷵻�false
        * ����		:  li.shugang
        *******************************************************************************/
        static bool TrySocket(const char* pszIP, int iPort);
        
    };
//}

#endif //__MINI_DATABASE_SOCKET_TRY_H__

