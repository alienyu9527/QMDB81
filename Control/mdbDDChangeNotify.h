/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbDDChangeNotify.h		
*@Description�� mdb�����ݶ���֪ͨ
*@Author:			jin.shaohua
*@Date��	    2013.2
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
        int GetDeleteTable(std::vector<std::string> & vDelTable);//��ȡɾ���ı�
    private:
        TMdbShmDSN * m_pShmDSN;
        std::map<std::string, int> m_arrTableState;//��״̬ 0-������1-����
    };
//}


#endif
