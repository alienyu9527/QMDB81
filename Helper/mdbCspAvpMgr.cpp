/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbCspAvpMgr.cpp		
*@Description：csp协议的解析
*@Author:	    jin.shaohua
*@Date：	    2012.06
*@History:
******************************************************************************************/
#include "Helper/mdbCspAvpMgr.h"
#include "Helper/TThreadLog.h"

//namespace QuickMDB{

    /******************************************************************************
    * 函数名称	:  TMdbCspParserMgr
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParserMgr::TMdbCspParserMgr()
    {
    	
    }

    /******************************************************************************
    * 函数名称	:  ~TMdbCspParserMgr
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParserMgr::~TMdbCspParserMgr()
    {
    	std::map<int ,TMdbCspParser * >::iterator itor = m_mapCspParser.begin();
    	for(;itor != m_mapCspParser.end();++itor)
    	{
    		SAFE_DELETE(itor->second);
    	}
    	m_mapCspParser.clear();
    }

    /******************************************************************************
    * 函数名称	:  GetParserByType
    * 函数描述	:  根据类型获取解析器，可以缓存已生成的解析器
    * 输入		:  bRequest - 是否是请求包  ture-请求包 false-响应包
    * 输入		:  
    * 输出		:  
    * 返回值	:  !NULL - 成功NULL -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParser * TMdbCspParserMgr::GetParserByType(int iCspParserType,bool bRequest)
    {
    	int iKey = 0;
    	iKey = (bRequest?1:-1) * iCspParserType;
    	if(0 == iKey)
    	{
    		TADD_ERROR(-1,"iCspParserType is 0.");
    		return 0;
    	}
    	std::map<int ,TMdbCspParser * >::iterator itor = m_mapCspParser.find(iKey);
    	if(itor != m_mapCspParser.end())
    	{
    	       itor->second->Clear();
    		return itor->second;
    	}
    	TMdbCspParser * pNewParser = new TMdbCspParser();
    	if(pNewParser->Init(iCspParserType,bRequest) != 0)
    	{//初始化失败，没有该类型的avp
    		SAFE_DELETE(pNewParser);
    		return NULL;
    	}
    	else
    	{
    		m_mapCspParser[iKey] = pNewParser;
    	}
           if(NULL != pNewParser){pNewParser->Clear();}
    	return pNewParser;
    }

//}

