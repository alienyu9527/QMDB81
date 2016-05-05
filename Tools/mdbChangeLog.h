/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbChangeLog.h		
*@Description： mdb版本变更日志记录
*@Author:			jin.shaohua
*@Date：	    2013.5
*@History:
******************************************************************************************/
#ifndef _MDB_CHANGE_LOG_H_
#define _MDB_CHANGE_LOG_H_

#include <string>

//namespace QuickMDB{

    //版本信息
    class TMdbVersionInfo
    {
    public:
        TMdbVersionInfo(const char * sVersion,const char * sData,const char * sUR,const char * sDesc):
            m_sVersion(sVersion),m_sDate(sData),m_sUR(sUR),m_sDescribe(sDesc){}
    public:
        std::string m_sVersion;   //版本号
        std::string m_sDate;        //Date
        std::string m_sUR;           //主要变更UR
        std::string m_sDescribe; //描述
    };

    //变更日志记录
    class TMdbChangeLog
    {
    public:
        void Print();//打印变更记录
    };


//}


#endif

