#include "Control/mdbLimitCtrl.h"

//namespace QuickMDB{
    TMDBLimitCtrl::TMDBLimitCtrl():
    m_iLimit(-1),
    m_iOffset(0)
    {

    }
    TMDBLimitCtrl::~TMDBLimitCtrl()
    {
    	
    }
    /******************************************************************************
    * 函数名称	:  
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMDBLimitCtrl::Init(ST_MEM_VALUE_LIST & stMemValueList)
    {
    	int iRet  = 0;
    	m_iLimit  = -1;//没有limit
    	m_iOffset = 0;
    	m_iCurPos = 0;
    	if(stMemValueList.iItemNum == 1)
    	{
    		m_iLimit = stMemValueList.vMemValue[0]->lValue;
    	}
    	else if(stMemValueList.iItemNum  == 2)
    	{
    		m_iLimit = stMemValueList.vMemValue[0]->lValue;
    		m_iOffset= stMemValueList.vMemValue[1]->lValue;
    	}
    	TADD_FUNC("m_iLimit=[%d],m_iOffset=[%d].",m_iLimit,m_iOffset);
    	return iRet;
    }
    /******************************************************************************
    * 函数名称	:  
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMDBLimitCtrl::DoNext()
    {
    	int iRet = DO_NEXT_TRUE;
    	if(0 > m_iLimit)
    	{
    		return DO_NEXT_TRUE;
    	}
    	else if(m_iCurPos >= m_iOffset && m_iCurPos < m_iOffset + m_iLimit)
    	{
    		iRet = DO_NEXT_TRUE;
    	}
    	else if(m_iCurPos < m_iOffset)
    	{
    		iRet = DO_NEXT_CONTINUE;
    	}
    	else if(m_iCurPos >= m_iOffset + m_iLimit)
    	{
    		iRet = DO_NEXT_END;
    	}
    	m_iCurPos ++;
    	return iRet;
    }
    /******************************************************************************
    * 函数名称	:  Clear
    * 函数描述	:  清理
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMDBLimitCtrl::Clear()
    {
    	m_iLimit  = -1;//没有limit
    	m_iOffset = 0;
    	m_iCurPos = 0;
    	return 0;
    }
//}
