#include "Helper/mdbMultiProtector.h"
#include "Helper/mdbOS.h"

//namespace QuickMDB{

    TMdbMultiProtector::TMdbMultiProtector():
    m_iPID(0),m_iTID(0)
    {

    }

    TMdbMultiProtector::~TMdbMultiProtector()
    {

    }
    
    //是否合法
    bool TMdbMultiProtector::IsValid()
    {
        if(m_iPID <= 0)
        {//首次链接
            m_iPID = TMdbOS::GetPID();
            m_iTID = (int) TMdbOS::GetTID();
        }
        if( TMdbOS::GetPID() != m_iPID || (int) TMdbOS::GetTID() != m_iTID )
        {//进程+ 线程不匹配
            return false;
        }
        return true;
    }
    //重置
    void TMdbMultiProtector::Reset()
    {
        m_iPID = 0;
        m_iTID = 0;
    }
    int TMdbMultiProtector::GetPID()
    {
        return m_iPID;
    }
    int TMdbMultiProtector::GetTID()
    {
        return m_iTID;
    }

//}
