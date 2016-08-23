#ifndef _MDB_INDEX_ALGO_H_
#define _MDB_INDEX_ALGO_H_

// 一个阶梯式索引节点体中包含的索引节点个数
const int  MAX_BODY_NODE_NUM= 8;

// 默认的阶梯式索引层数
const int DEFAULT_MHASH_LAYER = 2;

// 阶梯式索引的最大层数
const int MAX_MHASH_LAYER = 4;


// 阶梯式索引公用算法类
class TMdbMhashAlgo
{
public:
    
    static int CalcLayerPos(long long iHashValue)
    {
        return static_cast<int>(((iHashValue*3)%MAX_BODY_NODE_NUM));
    }
};

#endif
