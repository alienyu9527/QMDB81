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
    
    //�Ƿ�Ϸ�
    bool TMdbMultiProtector::IsValid()
    {
        if(m_iPID <= 0)
        {//�״�����
            m_iPID = TMdbOS::GetPID();
            m_iTID = (int) TMdbOS::GetTID();
        }
        if( TMdbOS::GetPID() != m_iPID || (int) TMdbOS::GetTID() != m_iTID )
        {//����+ �̲߳�ƥ��
            return false;
        }
        return true;
    }
    //����
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
