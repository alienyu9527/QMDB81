/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbDDChangeNotify.cpp		
*@Description： 
*@Author:			jin.shaohua
*@Date：	    2013.2
*@History:
******************************************************************************************/
#include "Control/mdbDDChangeNotify.h"

//namespace QuickMDB{

    TMdbDDChangeNotify::TMdbDDChangeNotify():
    m_pShmDSN(NULL)
    {

    }
    TMdbDDChangeNotify::~TMdbDDChangeNotify()
    {

    }
    /******************************************************************************
    * 函数名称	:  Attach
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbDDChangeNotify::Attach(const char * sDsn)
    {
        int iRet = 0;
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        //获取已存在的表
        m_arrTableState.clear();
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            if(itor->sTableName[0] != 0)
            {
                m_arrTableState[itor->sTableName] = 1;//存在
            }
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  GetDeleteTable
    * 函数描述	:  获取删除的表
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbDDChangeNotify::GetDeleteTable(std::vector<std::string> & vDelTable)
    {
        int iRet = 0;
        TMdbTable * pTable = NULL;
        vDelTable.clear();

        std::map<std::string, int>::iterator itor = m_arrTableState.begin();
        for(; itor != m_arrTableState.end(); itor++)
        {
            pTable = m_pShmDSN->GetTableByName(itor->first.c_str());
            if(0 != itor->second && (NULL == pTable || 0 == pTable->sTableName[0]))
            {
                vDelTable.push_back(itor->first.c_str());//已经被删除的表
                m_arrTableState[itor->first.c_str()] = 0;//不存在的表
            }
            else if(NULL !=pTable &&  pTable->sTableName[0] != 0)
            {
                m_arrTableState[pTable->sTableName] = 1;//存在的表
            }
        }
        return iRet;
    }
//}


