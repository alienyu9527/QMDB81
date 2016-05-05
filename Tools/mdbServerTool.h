/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbServerTools.h		
*@Description: mdbServer查询和命令发送工具
*@Author:		jiang.lili
*@Date：	    2015/01/28
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
        bool bHost;//是否打印主机信息
        bool bRouting;//是否打印路由信息
        bool bGroup;//是否打印分组组信息
        int iHostID;//待打印的主机ID，默认值为-1，全部打印
        int iRoutingID;//待打印的路由ID，默认值为-1，全部打印
        int iGroupID;//待打印的分组ID，默认值为-1， 全部打印
    };

    class TMdbServerTool
    {
    public:
        TMdbServerTool();
        ~TMdbServerTool();
    public:
        int Init();//初始化，连接mdbServer
        int ExecCmd(const ST_SERVERTOOL_PARAM &stParam);//执行命令
    private:
        void ShowHost(int iHostID);
        void ShowRouting(int iRoutingID);
        void ShowGroup(int iGroupID);
        
    private:
        TMdbRepNTCClient* m_pClient;
        
    };

//}
#endif