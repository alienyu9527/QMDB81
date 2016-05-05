/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbServerTools.h		
*@Description: mdbServer��ѯ������͹���
*@Author:		jiang.lili
*@Date��	    2015/01/28
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_SERVER_TOOL_H__
#define __ZTE_MINI_DATABASE_SERVER_TOOL_H__
#include "Replication/mdbRepNTC.h"
//namespace QuickMDB
//{
    struct ST_SERVERTOOL_PARAM
    {
    public:
        ST_SERVERTOOL_PARAM()
        {
            bHost = false;
            bRouting = false;
            bGroup = false;
            iHostID = -1;
            iGroupID = -1;
            iRoutingID = -1;
        }
        bool bHost;//�Ƿ��ӡ������Ϣ
        bool bRouting;//�Ƿ��ӡ·����Ϣ
        bool bGroup;//�Ƿ��ӡ��������Ϣ
        int iHostID;//����ӡ������ID��Ĭ��ֵΪ-1��ȫ����ӡ
        int iRoutingID;//����ӡ��·��ID��Ĭ��ֵΪ-1��ȫ����ӡ
        int iGroupID;//����ӡ�ķ���ID��Ĭ��ֵΪ-1�� ȫ����ӡ
    };

    class TMdbServerTool
    {
    public:
        TMdbServerTool();
        ~TMdbServerTool();
    public:
        int Init();//��ʼ��������mdbServer
        int ExecCmd(const ST_SERVERTOOL_PARAM &stParam);//ִ������
    private:
        void ShowHost(int iHostID);
        void ShowRouting(int iRoutingID);
        void ShowGroup(int iGroupID);
        
    private:
        TMdbRepNTCClient* m_pClient;
        
    };

//}
#endif