#ifndef _MDB_MULTI_PROTECTOR_H_
#define _MDB_MULTI_PROTECTOR_H_

//namespace QuickMDB{

    //��������
    class TMdbMultiProtector
    {
    public:
        TMdbMultiProtector();
        ~TMdbMultiProtector();    
        bool IsValid();//�Ƿ�Ϸ�
        void Reset();//����
        int GetPID();
        int GetTID();
    private:
        int m_iPID; //����id
        int m_iTID;//�߳�id
    };

//}

#endif
