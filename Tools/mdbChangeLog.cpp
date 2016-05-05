/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbChangeLog.cpp		
*@Description�� mdb�汾�����־��¼
*@Author:			jin.shaohua
*@Date��	    2013.5
*@History:
******************************************************************************************/
#include <stdio.h>
#include "Tools/mdbChangeLog.h"
#include <vector>

//namespace QuickMDB{

    //��Ӱ汾��Ϣ
#define ADD_VERSION_DESC(_version,_date,_ur,_desc) vVersionInfo.push_back(TMdbVersionInfo(_version,_date,_ur,_desc));

    /******************************************************************************
    * ��������	:  Print
    * ��������	:  ��ӡ�����¼
    * ����		:  
    * ���		:  
    * ����ֵ	:  
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbChangeLog::Print()
    {
        std::vector<TMdbVersionInfo> vVersionInfo;
        // TO ADD CHANGE LOGS
        std::vector<TMdbVersionInfo>::iterator itor = vVersionInfo.begin();
        printf("\nChange Log:\n");
        for(;itor != vVersionInfo.end();++itor)
        {
            TMdbVersionInfo & info = *itor;
            printf("[V%s.%s]\n",info.m_sVersion.c_str(),info.m_sDate.c_str());
            printf("\t UR=[%s],Desc=%s\n\n",info.m_sUR.c_str(),info.m_sDescribe.c_str());
        }
    }

//}

