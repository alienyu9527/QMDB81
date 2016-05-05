/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbDDChangeNotify.h		
*@Description： mdb的数据定义通知
*@Author:			jin.shaohua
*@Date：	    2013.2
*@History:
******************************************************************************************/
#ifndef _MDB_DD_CHANGE_NOTIFY_H_
#define _MDB_DD_CHANGE_NOTIFY_H_
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

    class TMdbDDChangeNotify
    {
    public:
        TMdbDDChangeNotify();
        ~TMdbDDChangeNotify();
        int Attach(const char * sDsn);//attach
        int GetDeleteTable(std::vector<std::string> & vDelTable);//获取删除的表
    private:
        TMdbShmDSN * m_pShmDSN;
        std::map<std::string, int> m_arrTableState;//表状态 0-不存在1-存在
    };
//}


#endif
