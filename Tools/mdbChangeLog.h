/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbChangeLog.h		
*@Description�� mdb�汾�����־��¼
*@Author:			jin.shaohua
*@Date��	    2013.5
*@History:
******************************************************************************************/
#ifndef _MDB_CHANGE_LOG_H_
#define _MDB_CHANGE_LOG_H_

#include <string>

//namespace QuickMDB{

    //�汾��Ϣ
    class TMdbVersionInfo
    {
    public:
        TMdbVersionInfo(const char * sVersion,const char * sData,const char * sUR,const char * sDesc):
            m_sVersion(sVersion),m_sDate(sData),m_sUR(sUR),m_sDescribe(sDesc){}
    public:
        std::string m_sVersion;   //�汾��
        std::string m_sDate;        //Date
        std::string m_sUR;           //��Ҫ���UR
        std::string m_sDescribe; //����
    };

    //�����־��¼
    class TMdbChangeLog
    {
    public:
        void Print();//��ӡ�����¼
    };


//}


#endif

