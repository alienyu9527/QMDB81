/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbDDChangeNotify.cpp		
*@Description�� 
*@Author:			jin.shaohua
*@Date��	    2013.2
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
    * ��������	:  Attach
    * ��������	:  
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbDDChangeNotify::Attach(const char * sDsn)
    {
        int iRet = 0;
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        //��ȡ�Ѵ��ڵı�
        m_arrTableState.clear();
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            if(itor->sTableName[0] != 0)
            {
                m_arrTableState[itor->sTableName] = 1;//����
            }
        }
        return iRet;
    }
    /******************************************************************************
    * ��������	:  GetDeleteTable
    * ��������	:  ��ȡɾ���ı�
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
                vDelTable.push_back(itor->first.c_str());//�Ѿ���ɾ���ı�
                m_arrTableState[itor->first.c_str()] = 0;//�����ڵı�
            }
            else if(NULL !=pTable &&  pTable->sTableName[0] != 0)
            {
                m_arrTableState[pTable->sTableName] = 1;//���ڵı�
            }
        }
        return iRet;
    }
//}


