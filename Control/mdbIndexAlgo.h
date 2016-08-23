#ifndef _MDB_INDEX_ALGO_H_
#define _MDB_INDEX_ALGO_H_

// һ������ʽ�����ڵ����а����������ڵ����
const int  MAX_BODY_NODE_NUM= 8;

// Ĭ�ϵĽ���ʽ��������
const int DEFAULT_MHASH_LAYER = 2;

// ����ʽ������������
const int MAX_MHASH_LAYER = 4;


// ����ʽ���������㷨��
class TMdbMhashAlgo
{
public:
    
    static int CalcLayerPos(long long iHashValue)
    {
        return static_cast<int>(((iHashValue*3)%MAX_BODY_NODE_NUM));
    }
};

#endif
