#ifndef _MDB_HSH_ALGO_H_
#define _MDB_HSH_ALGO_H_

#include "Helper/mdbStruct.h"

//namespace QuickMDB{

    // 哈希索引表定义
    struct  MDBHASHTABLE
    {
        unsigned long nHashA;
        unsigned long nHashB;
        bool bExists;
        //char test_filename[MAXSTRINGNAME];
        // char caValue[MAXSTRINGNAME]; //保留原始串
        // ......
    } ;

    // 对哈希索引表的算法进行封装
    class TMdbHashAlgo
    {
    public:
#if DEBUGTEST
        long  testid;   // 测试之用
#endif

        TMdbHashAlgo( const long nTableLength = MAXTABLELEN ) ;

        void prepareCryptTable();                                               // 对哈希索引表预处理

        inline unsigned long HashString(char *lpszString, unsigned long dwHashType)
        {
            unsigned char *key = (unsigned char *)(lpszString);
            unsigned long pot = (dwHashType << 8);

            unsigned long seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
            unsigned long ch;

            while(*key != 0)
            {
                ch = static_cast<unsigned long>((*key>='a' && *key<='z')? *key+('A'-'a') : *key);
                key++;

                seed1 = cryptTable[pot + ch] ^ (seed1 + seed2);
                seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
            }
            return seed1;

        }

        inline long GetHashTablePos( char *lpszString )                              // 得到在定长表中的位置
        {
            const unsigned long HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;
            unsigned long nHash = HashString(lpszString, HASH_OFFSET);
            unsigned long nHashA = HashString(lpszString, HASH_A);
            unsigned long nHashB = HashString(lpszString, HASH_B);
            unsigned long nHashStart = nHash % m_tablelength,   nHashPos = nHashStart;

            while(m_HashIndexTable[nHashPos].bExists)
            {
                if (m_HashIndexTable[nHashPos].nHashA == nHashA && m_HashIndexTable[nHashPos].nHashB == nHashB)
                    return static_cast<long>(nHashPos);
                else
                    nHashPos = (nHashPos + 1) % m_tablelength;

                if (nHashPos == nHashStart)
                    break;
            }

            return -1; //没有找到

        }

        long SetHashTable( char *lpszString );
        void ReSetHashTable();                               // 将字符串散列到哈希表中

        unsigned long GetTableLength(void);
        void SetTableLength( const unsigned long nLength );

        ~TMdbHashAlgo();

    private:
        unsigned long cryptTable[0x500];
        unsigned long m_tablelength;   // 哈希索引表长度
        MDBHASHTABLE m_HashIndexTable[MAXTABLELEN];
    };
//}


#endif
