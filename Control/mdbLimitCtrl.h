#ifndef _MDB_LIMIT_CTRL_H_
#define _MDB_LIMIT_CTRL_H_
#include <vector>
#include "Helper/SqlParserStruct.h"

//namespace QuickMDB{

    enum E_DO_NEXT
    {
    	DO_NEXT_TRUE = 1,
    	DO_NEXT_CONTINUE = 2,
    	DO_NEXT_END  = 3
    };
    //SQL 的limit offset控制
    class TMDBLimitCtrl
    {
    public:
    	TMDBLimitCtrl();
    	~TMDBLimitCtrl();
    	int Init(ST_MEM_VALUE_LIST & stMemValueList);
    	int DoNext();//是否做下一个
    	int Clear();//清理
    private:
    	MDB_INT64 m_iLimit;
    	MDB_INT64 m_iOffset;
    	MDB_INT64 m_iCurPos;//当前位置
    };
//}
#endif
