#ifndef _MDB_MULTI_PROTECTOR_H_
#define _MDB_MULTI_PROTECTOR_H_

//namespace QuickMDB{

    //并发保护
    class TMdbMultiProtector
    {
    public:
        TMdbMultiProtector();
        ~TMdbMultiProtector();    
        bool IsValid();//是否合法
        void Reset();//重置
        int GetPID();
        int GetTID();
    private:
        int m_iPID; //进程id
        int m_iTID;//线程id
    };

//}

#endif
