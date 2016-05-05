/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbCspAvpMgr.cpp		
*@Description��cspЭ��Ľ���
*@Author:	    jin.shaohua
*@Date��	    2012.06
*@History:
******************************************************************************************/
#include "Helper/mdbCspAvpMgr.h"
#include "Helper/TThreadLog.h"

//namespace QuickMDB{

    /******************************************************************************
    * ��������	:  TMdbCspParserMgr
    * ��������	:  
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbCspParserMgr::TMdbCspParserMgr()
    {
    	
    }

    /******************************************************************************
    * ��������	:  ~TMdbCspParserMgr
    * ��������	:  
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  GetParserByType
    * ��������	:  �������ͻ�ȡ�����������Ի��������ɵĽ�����
    * ����		:  bRequest - �Ƿ��������  ture-����� false-��Ӧ��
    * ����		:  
    * ���		:  
    * ����ֵ	:  !NULL - �ɹ�NULL -ʧ��
    * ����		:  jin.shaohua
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
    	{//��ʼ��ʧ�ܣ�û�и����͵�avp
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

