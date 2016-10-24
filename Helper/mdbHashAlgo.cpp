#include "Helper/mdbHashAlgo.h"

//namespace QuickMDB{

    void TMdbHashAlgo::prepareCryptTable()   
    {    
        unsigned long seed = 0x00100001, index1 = 0, index2 = 0, i;   
      
        for( index1 = 0; index1 < 0x100; index1++ )   
        {    
            for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )   
            {    
                unsigned long temp1, temp2;   
                seed = (seed * 125 + 3) % 0x2AAAAB;   
                temp1 = (seed & 0xFFFF) << 0x10;   
                seed = (seed * 125 + 3) % 0x2AAAAB;   
                temp2 = (seed & 0xFFFF);   
                cryptTable[index2] = ( temp1 | temp2 );    
            }    
        }    
    }   

    TMdbHashAlgo::TMdbHashAlgo( const long nTableLength ) // 创建指定大小的哈希索引表，不带参数的构造函数创建默认大小的哈希索引表   
    {   
            prepareCryptTable();   
            m_tablelength = (long unsigned)(nTableLength);   
               
            //m_HashIndexTable = new MDBHASHTABLE[nTableLength];   
            for ( int i = 0; i < MAXTABLELEN; i++ )   
            {   
                m_HashIndexTable[i].nHashA = 0;   
                m_HashIndexTable[i].nHashB = 0;   
                m_HashIndexTable[i].bExists = false;   
               // m_HashIndexTable[i].caValue[0] = '\0';   
            }           
    }  

    void TMdbHashAlgo::ReSetHashTable()
    {
    	for ( int i = 0; i < MAXTABLELEN; i++ )   
            {   
                m_HashIndexTable[i].nHashA = 0;   
                m_HashIndexTable[i].nHashB = 0;   
                m_HashIndexTable[i].bExists = false;   
               // m_HashIndexTable[i].caValue[0] = '\0';   
            }      
    }
     

    TMdbHashAlgo::~TMdbHashAlgo()   
    {   
            //if ( NULL != m_HashIndexTable )   
            //{   
              //  delete []m_HashIndexTable;   
                //m_HashIndexTable = NULL;   
                m_tablelength = 0;   
           // }   
    }   
      
    //////////////////////////////////////////////////////////////////////////   
    // 求取哈希值 
    /*  
    void TMdbHashAlgo::HashString(char *lpszString, unsigned long dwHashType,unsigned long &nHash,unsigned long &nHashA)   
    {    
    	//TMdbStrFunc::ToUpper((const char*)lpszString,lpszString);
        unsigned char *key = (unsigned char *)lpszString; 
       
        register unsigned long pot = (dwHashType << 8);
        //unsigned long pot1 = (dwHashTypeA << 8);
        
        register unsigned long seed1 = 0x7FED7FED;
        register unsigned long seed2 = 0xEEEEEEEE;   
        //unsigned long seed3 = 0x7FED7FED, seed4 = 0xEEEEEEEE;   
        register unsigned long seed = 131; // 31 131 1313 13131 131313 etc..
        register unsigned long hash = 0;
        register int ch;   
        //int iStep = -32;
        while(*key != 0)   
        {    
            //<a, >z
            ch = (*key < 0x61 || *key > 0x7A)? *key : *key&0xDF; 
            key++;
            
            seed1 = cryptTable[pot + ch] ^ (seed1 + seed2);   
            seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;  
            
            hash = hash * seed + (ch);
            
            //seed3 = cryptTable[pot1 + ch] ^ (seed3 + seed4);   
            //seed4 = ch + seed3 + seed4 + (seed4 << 5) + 3;   
        } 
        
          
        nHash = seed1;    
        nHashA = hash & 0x7FFFFFFF;
    }   
      */
    //////////////////////////////////////////////////////////////////////////   
    // 得到在定长表中的位置  
    /* 
    long TMdbHashAlgo::GetHashTablePos(char *lpszString)   
    {    
        const unsigned long HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;   
        unsigned long nHash = 0 ;   
        unsigned long nHashA= 0;
        HashString(lpszString, HASH_OFFSET,nHash,nHashA);
        //= HashString(lpszString, HASH_A);   
        //unsigned long nHashB = HashString(lpszString, HASH_B);   
        unsigned long nHashStart = nHash % m_tablelength,   nHashPos = nHashStart;   
      
        while ( m_HashIndexTable[nHashPos].bExists)   
        {    
            //if (m_HashIndexTable[nHashPos].nHashA == nHashA && m_HashIndexTable[nHashPos].nHashB == nHashB) 
            if (m_HashIndexTable[nHashPos].nHashA == nHashA)       
                return nHashPos;    
            else    
                nHashPos = (nHashPos + 1) % m_tablelength;   
      
            if (nHashPos == nHashStart)    
                break;    
        }   
      
        return -1; //没有找到   
    }   
    */
    //////////////////////////////////////////////////////////////////////////   
    // 通过传入字符串，将相应的表项散列到索引表相应位置中去   
    long TMdbHashAlgo::SetHashTable( char *lpszString )   
    {   
    	/*
        const unsigned long HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;   
        unsigned long nHash = 0;// HashString(lpszString, HASH_OFFSET);   
        unsigned long nHashA = 0;//HashString(lpszString, HASH_A);  
        HashString(lpszString, HASH_OFFSET,nHash,nHashA); 
        //unsigned long nHashB = HashString(lpszString, HASH_B);   
        unsigned long nHashStart = nHash % m_tablelength, nHashPos = nHashStart;   
        
        while ( m_HashIndexTable[nHashPos].bExists)   
        {    
            nHashPos = (nHashPos + 1) % m_tablelength;   
            if (nHashPos == nHashStart)    
            {        
                return -1;
            }   
        }   
        m_HashIndexTable[nHashPos].bExists = true;  
        m_HashIndexTable[nHashPos].nHashA = nHashA;
        //m_HashIndexTable[nHashPos].nHashA = nHashA;   
        //m_HashIndexTable[nHashPos].nHashB = nHashB;   
        //strcpy( m_HashIndexTable[nHashPos].caValue, lpszString );   
      
     
      
        return nHashPos;
        */
        
        const unsigned long HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;   
        unsigned long nHash = HashString(lpszString, HASH_OFFSET);   
        unsigned long nHashA = HashString(lpszString, HASH_A);   
        unsigned long nHashB = HashString(lpszString, HASH_B);   
        unsigned long nHashStart = nHash % m_tablelength, nHashPos = nHashStart;   
      
        while ( m_HashIndexTable[nHashPos].bExists)   
        {    
            nHashPos = (nHashPos + 1) % m_tablelength;   
            if (nHashPos == nHashStart)    
            {    
      
                //return false;    
                return -1;
            }   
        }   
        m_HashIndexTable[nHashPos].bExists = true;   
        m_HashIndexTable[nHashPos].nHashA = nHashA;   
        m_HashIndexTable[nHashPos].nHashB = nHashB;   
        //strcpy( m_HashIndexTable[nHashPos].caValue, lpszString );   
      
      
        return (long)(nHashPos);   
           
    }   
      
    //////////////////////////////////////////////////////////////////////////   
    // 取得哈希索引表长   
    unsigned long TMdbHashAlgo::GetTableLength(void)   
    {   
        return m_tablelength;   
    }   
      
    //////////////////////////////////////////////////////////////////////////   
    // 设置哈希索引表长   
    void TMdbHashAlgo::SetTableLength( const unsigned long nLength )   
    {   
        m_tablelength = nLength;   
        return;   
    }  


    

//}
