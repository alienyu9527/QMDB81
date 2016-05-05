/*
 * CS模式登录协议封装
 * 作者: jiang.mingjun
 */

#ifndef __ZTE_QUICK_MEMORY_DATABASE_LONGON_MGR__H__
#define __ZTE_QUICK_MEMORY_DATABASE_LONGON_MGR__H__

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "Helper/mdbCSPParser.h"
#include <map>

//namespace QuickMDB{

    //csp解析器管理器
    class TMdbCspParserMgr
    {
    public:
    	TMdbCspParserMgr();
    	~TMdbCspParserMgr();
    	TMdbCspParser * GetParserByType(int iCspParserType,bool bRequest);
    private:
    	std::map<int ,TMdbCspParser * > m_mapCspParser;//cspparser
    };
//}

#endif

