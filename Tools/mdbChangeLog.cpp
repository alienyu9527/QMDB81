/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbChangeLog.cpp		
*@Description： mdb版本变更日志记录
*@Author:			jin.shaohua
*@Date：	    2013.5
*@History:
******************************************************************************************/
#include <stdio.h>
#include "Tools/mdbChangeLog.h"
#include <vector>

//namespace QuickMDB{

    //添加版本信息
#define ADD_VERSION_DESC(_version,_date,_ur,_desc) vVersionInfo.push_back(TMdbVersionInfo(_version,_date,_ur,_desc));

    /******************************************************************************
    * 函数名称	:  Print
    * 函数描述	:  打印变更记录
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
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

