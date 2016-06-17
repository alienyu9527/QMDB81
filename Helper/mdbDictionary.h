/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbDictionary.h		
*@Description： mdb字典定义
*@Author:		jin.shaohua
*@Date：	    2014/02/28
*@History:
******************************************************************************************/
#ifndef _MDB_DICTIONARY_H_
#define _MDB_DICTIONARY_H_
#include "Interface/mdbQuery.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbBitMap.h"
#include "Helper/TMutex.h"
#include "Helper/mdbHashAlgo.h"
#include <vector>
#include "Helper/mdbJson.h"
#include "Helper/mdbShmSTL.h"

//namespace QuickMDB{

#define MAX_FLUSN_DAO_COUNTS 30
#define MAX_MDB_COLUMN 100 //mdb最多列数
#define MAX_MDB_INDEX 10    //mdb最多索引个数
#define MAX_BIT_MAP_SIZE (10240) //bitmap大小单位是字节
#define MAX_BLOB_LEN (8192)   //blob数据类型
#define MAX_SQL_LEN (8192)   //SQL最大长度
#define MAX_INDEX_COLUMN_NAME  (128)  //索引列名列表长度
#define MAX_TABLESPACE_COUNT (256) //最多表空间个数
#define MAX_FLUSH_CHANGED_PAGE_COUNT (100) //最多一次写入临时文件的脏页个数

#define MAX_MDB_ROWID_SIZE  (sizeof(int))   //rowid大小(page-id + dataoffset)，4 字节
#define MAX_MDB_PAGE_COUNT    (2097151)      //每个表空间最大页个数，占rowid的21位
#define MAX_MDB_PAGE_RECORD_COUNT (2047) //页内最大记录个数占rowid的11位
#define MAX_VALUE_LEN      (32768)  //数据最大长度
#define TS_FILE_PREFIX "TS_"      //文件前缀
#define VARCHAR_FILE_PREFIX "VARCHAR_"      //文件前缀
#define TS_FILE_SUFFIX ".mdb"   //文件后缀
#define TS_CHANGE_FILE_SUFFIX ".change"   //临时文件后缀
#define BAK_FILE_SUFFIX ".bak"   //临时文件后缀
//QMDB支持的数据类型有如下:
#define DT_Unknown   0 //未知类型
#define DT_Int       1 //可以写成Int(4)、Int(8)、Int(11)等类似形式，默认是Int(8)
#define DT_Char      2 //字符型，固定长度，例如Char(2)表示有2个字符，系统内部会自动控制
#define DT_VarChar   3 //变长数据类型	
#define DT_DateStamp 4 //日期类型，格式为’YYYYMMDDHHM24SS’
#define DT_Blob      9 //BLOB类型，二进制表示
#define MAX_INDEX_COUNTS  10

// 阶梯式索引最大阶层数
#define MAX_INDEX_LAYER_LIMIT 4

#define MAX_BASE_INDEX_COUNTS 500

#define MAX_MHASH_INDEX_COUNT 1000
#define MAX_MHASH_SHMID_COUNT 1000

#define MAX_BRIE_INDEX_COUNT  100
#define MAX_BRIE_SHMID_COUNT 100

#define MAX_TRIE_WORD_LEN    256


//VARCHAR 支持的段
#define VC_16      0
#define VC_32      1
#define VC_64      2
#define VC_128     3
#define VC_256     4
#define VC_512     5
#define VC_1024    6
#define VC_2048    7
#define VC_4096    8
#define VC_8192    9

	/**
	 * @brief rowid 一条记录的唯一标识
	 * 
	 */
	class TMdbRowID
        {
        public:
            void Clear()
            {
                m_iRowID = 0;
            }
            //获取页号
            int GetPageID();
            //获取数据偏移
            int GetDataOffset();
            //设置pageid
            int SetPageID(int iPageID);
            int SetDataOffset(int iDataOffset);
			int SetRowId(unsigned int iRowId){m_iRowID = iRowId;return 0;}
            bool IsEmpty(){return 0==m_iRowID;}
            bool operator==(const TMdbRowID &t1)const{
                 return (this->m_iRowID == t1.m_iRowID);
             }  
			unsigned int m_iRowID;
        private:
            
           // int iPageID;  //所属页号
           // int iDataOffset;     //在页中的数据偏移
        };


	enum DATA_FLAG
	{
		DATA_REAL = 0,
		DATA_VIRTUAL = 0x01,
		DATA_DELETE = 0x02,
		DATA_RECYCLE = 0x04
	};
	/**
	 * @brief 页节点
	 * 
	 */
	class TMdbPageNode
	{
	public:
		void Print(){printf("SessionID:%d,cFlag:%d\n",iSessionID,cFlag);}
	public:
		char   cStorage;  //这条记录是否是文件存储的，主要用于varchar，非varchar页不使用该字段 Y:是 N:不是
		int    iNextNode; //下一个节点，用来做连表，可以存放未使用的节点 <0 表示该节点正在使用，>=0 表示该节点空闲
		int    iPreNode; //前一个节点
		TMutex tMutex;
		unsigned int iSessionID;
		unsigned char cFlag;
	};


	/**
	 * @brief 
	 * 
	 */
	class TMdbPage
        {
        public:
            void Init(int iPageID, int iPageSize);
			void Init(int iPageID, int iPageSize, int iVarcharID);//初始化varchar页
            int GetFreeRecord(int & iNewDataOffset,int iDataSize);//获取一个空闲的记录空间
            int PushBack(int iDataOffset);//归还一个记录空间
            std::string ToString();//输出
            //是否是合法的dataOffset
            bool IsValidDataOffset(int iDataOffset)
            {
                return true == (iDataOffset < m_iFreeOffSet && iDataOffset >= (int)sizeof(TMdbPage) + (int)sizeof(TMdbPageNode));
            }
            bool bNeedToMoveToFreeList(){return (m_iFreeSize * 4 >= m_iPageSize)?true:false;};//是否空闲
            char * GetNextDataAddr(char * &pCurDataAddr,int & iDataOffset,char *&pNextDataAddr);//获取下一个数据地址
            //记录位置转data offset
            int RecordPosToDataOffset(int iRecordPos);
            //data offset 转记录位置
            int DataOffsetToRecordPos(int iDataOffset);
			int LockRow(int iDataOffset,char* pHeadAddr);
			int UnLockRow(int iDataOffset,char* pHeadAddr);
			
        public:
            char m_sUpdateTime[MAX_TIME_LEN];    //变更时间
            int m_iPageID;          //页号
            int m_iPrePageID;       //上一个页号, 组成一个双向连表，可以进行方便的移动
            int m_iNextPageID;      //下一个页号
            int m_iRecordCounts;    //总记录数
            int m_iPageSize;        //页大小,默认为4096
            int m_iFreePageNode; //第一个空闲的页节点
            int m_iFullPageNode;//最后一个满节点，也就是把有数据的节点串联起来
            int m_iFreeOffSet;   //空闲偏移量
            int m_iRecordSize;  //数据的长度[数据长度是定长的]
            char m_sState[8];     //状态：full-已满; free-空闲; empty-空; move -页面迁移
            int m_iFreeSize;      //空闲空间
            char m_sTableName[MAX_NAME_LEN]; //当前页属于哪个表
            //int m_iTableID;//当前属于哪个表ID
            long long m_iPageLSN;//页操作号
        };


	//Table-Space 节点
	class TMdbTSNode
        {
        public:
            TMdbTSNode();
            void Clear();
            int iPageStart;  //起始页号
            int iPageEnd;    //结束页号
            SHAMEM_T iShmID;   //所属共享内存ID
            size_t iOffSet;  //起始页在共享内存中的偏移量
            size_t iNext;       //下一个节点偏移量
            char bmDirtyPage[MAX_BIT_MAP_SIZE];//脏页表,bitmap
        };


	/**
	 * @brief 表空间
	 * 
	 */
	class TMdbTableSpace
        {
        public:
            TMdbTableSpace(){Clear();}
            ~TMdbTableSpace() {}
            void Clear();
            void Print();
            void Show(bool bIfMore=false);
            bool m_bFileStorage; // 是否是文件存储的表空间
            int m_iBlockId; //文件存储块号
            int m_iChangeBlockId; //文件存储临时文件块号
            char sName[MAX_NAME_LEN];      //表空间名称
            size_t  iPageSize;       //页大小，默认为4096
            int  iRequestCounts;  //一次申请的页面数，默认为1万
            int  iEmptyPages;     //空闲页数
            int  iTotalPages;     //总的页数
            TMdbTSNode tNode;     //节点信息
            int  iEmptyPageID;    //空闲页的第一个页
            TMutex tEmptyMutex;   //空闲页的共享锁
            TMutex m_tFreeMutex; // 自由页的共享锁
            TMutex m_tFullMutex; // 满页的共享锁
            char cState;      //表空间状态：’0’-未创建;’1’-已经创建;’2’-正在创建;’3’正在销毁
            char sCreateTime[MAX_TIME_LEN]; //表空间创建时间
        };

	/*varchar 管理区*/
	class TMdbVarchar
	{
	public:
		TMdbVarchar();
		~TMdbVarchar();
		void Clear();
		int iVarcharID;   //表示是哪个段的，16/32/64/128等
		int iBlockId;      //文件存储块号，1个文件大小为1G
		int iChangeBlockId; 
		size_t iPageSize; //页面大小，页面大小根据段来确定
		int iTotalPages;     //总页数
		int iFreePageId; //空闲链
		int iFullPageId; //满链
		TMdbTSNode tNode;     //头节点
		TMutex tMutex;   //空闲页的共享锁
	};
	/**
	 * @brief mdb的列定义
	 * 
	 */

	class TMdbColumn
        {
        public:
            void Clear();
            void Print();
            bool bIncrementalUpdate();//是否是增量更新列
            bool IsStrDataType();//是否是字符串类型的字段
            char sName[MAX_NAME_LEN]; //列名称
            int  iDataType;           //列的数据类型
            int  iColumnLen; 	      //列的数据长度，如果是变长数据则为-1
            int  iPos;                //列的位置, 从0...n
            int  iOffSet;             //数据的偏移量
            bool isInOra;             //Oracle中是否存在本字段,默认是true，如果这个字段是session类的，则为false
            long iHashPos;            //节点在hash表中的位置
            bool bIsDefault;
            char iDefaultValue[256];
            bool m_bNullable;//是否允许null;
        };
	/**
	 * @brief 索引类型
	 * 
	 */
	enum E_MDB_INDEX_TYPE
	{
		INDEX_HASH = 0,     //hash索引
		INDEX_M_HASH    = 1,     //multistep hash index
		INDEX_TRIE = 2,     //trie索引
		INDEX_UNKOWN 
	};

	// 表的记录数级别
	enum E_MDB_TABLE_LEVEL
	{
		TAB_TINY = 1, // 1w级别
		TAB_MINI =2, // 50w
		TAB_SMALL =3, // 200w
		TAB_LARGE = 4, // 1000w
		TAB_HUGE = 5, // 5000w
		TAB_ENORMOUS // 10000w
	};

	/**
	 * @brief 索引信息
	 * 
	 */
	class TMdbIndex
        {
        public:
            TMdbIndex() {}
            ~TMdbIndex() {}
            void Clear();
            void Print();
            const char* GetAlgoType();
            int GetInnerAlgoType(const char* psAlgoName);

            char sName[MAX_NAME_LEN]; //索引名称
            int m_iAlgoType;  // 索引的算法类型: hash , multistep hash, btree
            int m_iIndexType; // 索引的类型: int , char, composite index
            int iColumnNo[MAX_INDEX_COLUMN_COUNTS];       //索引作用在列的序列号（从0开始）
            int iPriority;       //索引优先级,1…10，数值越小优先级越高
            //TMutex tMutex;       //索引的共享锁
            int iMaxLayer;
			bool bBuilding;
        };


    //主键定义
class TMdbPrimaryKey
{
public:
    void Clear();
    void Print();

    int iColumnCounts;                 //主键需要的列数
    int iColumnNo[MAX_PRIMARY_KEY]; //主键对应的列号
    char m_sTableName[MAX_NAME_LEN] ;
};

    class TMdbMTableAttr
{
public:
    void Clear();

    char sAttrName[MAX_NAME_LEN];   //属性名
    char sAttrValue[1024];        //属性值
    bool bIsExist;                 //属性是否存在
};


class TMdbParameter
{
public:
    void Clear();
    int iDataType;
    char sName[MAX_NAME_LEN];
    char sValue[256];
    int iParameterType;
};



    //变长数据的存储域
    class TvarcharNode
    {
    public:
        TvarcharNode()
        {
            Clear();
        }
        ~TvarcharNode()
        {
            Clear();
        }
        void Clear()
        {
            iShmNo = -1;
            iOffSet = -1;
            iSize = -1;
            //m_bIsAttach = false;
            m_bState = false;
        }
    public:
        int  iShmNo;   //共享内存的序列号，这里并不使用ID,目的为了快速访问
        int  iOffSet;   //在共享内存中的偏移量
        int  iSize;     //在共享内存中的大小，单位是M
        //bool m_bIsAttach;
        bool m_bState;  //是否已经满了.
    };

    //共享内存块的头结构, 仅存在于非管理区和非基础索引区中
    class TMdbShmHead
    {
    public:
        SHAMEM_T iShmID;  //本共享内存段的ID
        size_t iTotalSize;  //总大小
        size_t iLeftOffSet; //剩余空间偏移量
        size_t iAdvOffSet;  //矫正内存偏移

        int iDataLen;    //用于变长存储区头信息,记录存储的是哪个长度的.
        int iRecordCounts; //当前变长存储区里存放的记录数.
        int iTotalRecords; //该变长存储区一共能存放的记录数.

        //vector<long> vfreeList; //记录空闲的记录数位置
        int iFirstFreeRecord;    //记录最前面的空闲的记录数位置

        TMutex tMutex;    //管理区共享锁

        //取得一块内存iSize-请求大小，iReal-实际大小，iPageSize-页面大小，iPageCounts-最少页面数
        size_t GetMemory(size_t iSize, size_t &iReal, size_t iPageSize, size_t iPageCounts)
        {
            tMutex.Lock(true);

            size_t iRet = 0;

            if(iTotalSize - iLeftOffSet >= iSize)
            {
                iReal = iSize;
                iLeftOffSet += iReal;
                iRet = iLeftOffSet-iReal;
            }
            else if(iTotalSize - iLeftOffSet >= iPageSize * iPageCounts)
            {
                iReal = (iTotalSize - iLeftOffSet)/iPageSize*iPageSize;
                iLeftOffSet += iReal;
                iRet = iLeftOffSet-iReal;
            }
            else
            {
                iRet = 0;
            }

            tMutex.UnLock(true);

            return iRet;
        }
    };

    class TvarcharBlock
    {
    public:
        TvarcharBlock()
        {
            Clear();
        }

        ~TvarcharBlock()
        {
            Clear();
        }

        void Clear();

    public:
        char sName[128];                            //记录这个域的名称，用作正确性校验。
        int  iShmID[MAX_VARCHAR_SHM_ID];           //记录10000个共享内存块，通常是足够了。
        TvarcharNode tVarchar16[1000];
        TvarcharNode tVarchar32[1000];
        TvarcharNode tVarchar64[1000];
        TvarcharNode tVarchar128[1000];
        TvarcharNode tVarchar256[1000];
        TvarcharNode tVarchar512[1000];
        TvarcharNode tVarchar1024[1000];
        TvarcharNode tVarchar2048[1000];
        TvarcharNode tVarchar4096[1000];
        TvarcharNode tVarchar8192[1000];
        
        int iOffSet;                     //下一个域节点的位置

        //一共1000个块,当前使用到哪个块.
        int m_iCurrentPos16;
        int m_iCurrentPos32;
        int m_iCurrentPos64;
        int m_iCurrentPos128;
        int m_iCurrentPos256;
        int m_iCurrentPos512;
        int m_iCurrentPos1024;
        int m_iCurrentPos2048;
        int m_iCurrentPos4096;
        int m_iCurrentPos8192;

        int m_iFreeNode16[1000];
        int m_iFreeNode32[1000];
        int m_iFreeNode64[1000];
        int m_iFreeNode128[1000];
        int m_iFreeNode256[1000];
        int m_iFreeNode512[1000];
        int m_iFreeNode1024[1000];
        int m_iFreeNode2048[1000];
        int m_iFreeNode4096[1000];
        int m_iFreeNode8192[1000];

        int m_iMaxFreeNode16;
        int m_iMaxFreeNode32;
        int m_iMaxFreeNode64;
        int m_iMaxFreeNode128;
        int m_iMaxFreeNode256;
        int m_iMaxFreeNode512;
        int m_iMaxFreeNode1024;
        int m_iMaxFreeNode2048;
        int m_iMaxFreeNode4096;
        int m_iMaxFreeNode8192;

        int m_iNumFreeNode16;
        int m_iNumFreeNode32;
        int m_iNumFreeNode64;
        int m_iNumFreeNode128;
        int m_iNumFreeNode256;
        int m_iNumFreeNode512;
        int m_iNumFreeNode1024;
        int m_iNumFreeNode2048;
        int m_iNumFreeNode4096;
        int m_iNumFreeNode8192;

        TMutex tVarCharMutex;
    };

    //索引节点
    class TMdbIndexNode
    {
        public:
        TMdbRowID tData;  //数据所在位置
        int       iNextPos;//下一个node 的位置
        // int 	iPrePos;//前一个node的位置
    };

    // 索引内存块上的空闲块，用于内存回收
    class TMDBIndexFreeSpace
    {
    public:
        void Clear()
        {
            iPosAdd = 0;
            iSize   = 0;
        }
        size_t iPosAdd;  //空闲块偏移量
        size_t iSize;	  //空闲块大小
    };
    //基础索引信息
    class TMdbBaseIndex
    {
    public:
        void Clear()
        {
            memset(sName, 0, sizeof(sName));
            iPosAdd = 0;
            iSize   = 0;
            cState  = '0';
            memset(sTabName, 0, sizeof(sTabName));
            iConflictMgrPos = -1;
            iConflictIndexPos = -1;
            memset(sCreateTime, 0, sizeof(sCreateTime));
        }
        char sName[MAX_NAME_LEN];   //索引名称
        char sTabName[MAX_NAME_LEN]; // 表名
        size_t  iPosAdd;               //索引偏移量
        size_t  iSize;                 //基础索引总体大小，单位为字节
        char cState;                //索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁;
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iConflictMgrPos;         //冲突管理区的pos
        int  iConflictIndexPos;     //冲突索引pos
    };


    //基础索引管理区信息
    class TMdbBaseIndexMgrInfo
    {
    public:
        int iSeq;         //第几个共享内存段
        TMutex tMutex;    //管理区共享锁
        TMdbBaseIndex tIndex[MAX_BASE_INDEX_COUNTS]; //基础索引信息
        int iIndexCounts;  //已有索引数
        TMDBIndexFreeSpace tFreeSpace[MAX_BASE_INDEX_COUNTS];//空闲空间
        MDB_INT64 iTotalSize;   //总大小
    };

    //冲突索引信息
    class TMdbConflictIndex
    {
    public:
        void Clear()
        {
            iPosAdd = 0;
            iSize   = 0;
            cState  = '0';
            memset(sCreateTime, 0, sizeof(sCreateTime));
            iFreeHeadPos = -1;
            iFreeNodeCounts = 0;
        }
        size_t GetTotalCount(){return iSize/sizeof(TMdbIndexNode);}//总个数
        size_t iPosAdd;               //索引偏移量
        size_t  iSize;                 //索引总体大小，单位为字节
        char cState;                //索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iFreeHeadPos;         //空闲头结点位置
        int  iFreeNodeCounts;      //剩余空闲节点数
    };

    //冲突索引管理区信息
    class TMdbConflictIndexMgrInfo
    {
    public:
        int iSeq;         //第几个共享内存段
        TMutex tMutex;    //管理区共享锁
        TMdbConflictIndex tIndex[MAX_BASE_INDEX_COUNTS]; //基础索引信息
        int iIndexCounts;  //已有索引数
        TMDBIndexFreeSpace tFreeSpace[MAX_BASE_INDEX_COUNTS];//空闲空间
        MDB_INT64 iTotalSize;   //总大小
    };

    

    //mdb每条记录的每个索引节点的位置
    class TMdbRowIndex
    {
    public:
        void Clear(){iBaseIndexPos = -1;iConflictIndexPos = -1;iPreIndexPos = -1;}
        bool IsPreNodeInConflict(){return iPreIndexPos >= 0?true:false;}   //前置节点是否在冲突链上
        bool IsCurNodeInConflict(){return iConflictIndexPos >= 0?true:false;}//当前节点是否在冲突链上
    public:
        int iBaseIndexPos;     //基础索引节点位置
        int iConflictIndexPos;//冲突索引节点位置 < 0表示不在冲突链上
        int iPreIndexPos;       //前一个节点位置     < 0 表示不在冲突链上
    };

    
//数据库中的的进程信息
class TMdbProc
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    bool IsValid(){return (0 != sName[0] && 0 != sStartTime[0]);}
public:
    char sName[MAX_NAME_LEN];      //进程名称
    char cState;                   //进程状态:'S'-停止; 'F'-空闲; 'B'-忙碌
    int  iPid;                     //进程在系统中的PID
    char sStartTime[MAX_TIME_LEN]; //进程启动时间
    char sStopTime[MAX_TIME_LEN];  //进程停止时间
    char sUpdateTime[MAX_TIME_LEN]; //进程更新时间
    int  iLogLevel;                 //进程的日志级别
    bool bIsMonitor;                //是否需要监控
    int Serialize(MDB_JSON_WRITER_DEF & writer);//序列化
};

class TMdbPortLink
{
	public:
    void Clear(){iAgentPort = -1; iConNum = 0;}
    void Print(){printf("agent port:%d,connect link num:%d\n",iAgentPort,iConNum); }
	void AddConNum(){iConNum++;}
	void SubConNum(){iConNum--; if(iConNum<0)iConNum = 0;}
	public:
	int iAgentPort;
	int iConNum;

};

//数据库远程链接信息
class TMdbRemoteLink
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    bool IsValid(){return ( 0 != sIP[0] && 0 != sStartTime[0]);}
    bool IsCurrentThreadLink();//是否是当前线程的链接
public:
    char sIP[MAX_IP_LEN]; //进程的IP
    int iHandle;            //链接的句柄
    char cState;          //链接状态
    char sStartTime[MAX_TIME_LEN]; //链接启动时间
    char sFreshTime[MAX_TIME_LEN]; //链接最近更新时间，相当于心跳
    int  iLogLevel;                //链接的日志级别
    char cAccess;                  //权限, A-管理员;W-可读写;R-只读
    char sUser[MAX_NAME_LEN];      //用户名
    char sPass[MAX_NAME_LEN];      //密码
    char sDSN[MAX_NAME_LEN];       //DSN名称
    int iLowPriority;              //优先级
    int iPID;                      //进程的PID
    unsigned long iTID; 			//远程链接的线程ID
    int iSQLPos;                    //远程链接上sql位置
    int iProtocol;                  //协议
    char sProcessName[MAX_NAME_LEN];   //哪个进程触发链接
};

class TMdbCSLink
{//CS 连接注册信息
public:
    TMdbCSLink()
    {
        clear();
    }

    void clear()
    {
        iFD           = -1;
        m_pRemoteLink = NULL;
        m_sUser       = NULL;
        m_sPass       = NULL;
        m_sDSN        = NULL;
        m_iClientPID  = -1;
        m_iClientTID  = -1;
		m_iLowPriority = 0;
        m_sClientIP[0]= '\0';
        m_iUseOcp = MDB_CS_USE_OCP;
        memset(&tAddr,0,sizeof(tAddr));
    }
public:
    int iFD; //客户端socket链接ID
    struct sockaddr_in tAddr;
    TMdbRemoteLink * m_pRemoteLink;
    char *m_sUser;
    char *m_sPass;
    char *m_sDSN;
    char m_sClientIP[MAX_IP_LEN];
    int m_iClientPID;//客户端PID
    int m_iClientTID;//客户端线程ID
    int m_iLowPriority;//优先级
    int m_iUseOcp;//协议    
};

class TMdbRepLink
{
public:
    void Clear();
	void Print();
    void Show();
    int iSocketID;
    int iPID;
    char sIP[MAX_IP_LEN];
    int iPort;
    char sStartTime[MAX_TIME_LEN];
    char sState;
    char sLink_Type;
};

//job类型,年，月，日，小时，分钟,秒
enum E_JOB_RATE_TYPE
{
    JOB_PER_NONE =0,//无效频率
    JOB_PER_YEAR = 1,
    JOB_PER_MONTH,
    JOB_PER_DAY,
    JOB_PER_HOUR,
    JOB_PER_MIN,
    JOB_PER_SEC
};


//job状态
enum E_JOB_STATE
{
   JOB_STATE_NONE = 1,//无效状态
   JOB_STATE_WAIT ,    //等待执行
   JOB_STATE_RUNNING //正在执行
};

class TMdbJob
{
public:
    TMdbJob();
    int Clear();//清理
    int SetRateType(const char * sRateType);//设置RateType
    int GetRateType(char*pRateType,const int iLen);//获取RateType
    std::string ToString();//打印
    bool IsValid(){return JOB_PER_NONE != m_iRateType && 0 != m_sSQL[0] && 0 != m_iInterval;}//是否是有效的job
    void StateToWait(){m_iState = JOB_STATE_WAIT;}//变更状态到等待态
    void StateToRunning(){m_iState = JOB_STATE_RUNNING;};//变更状态到执行态
    bool IsCanModify(){return JOB_STATE_RUNNING != m_iState;};//是否可以变更。
public:
    char m_sName[MAX_NAME_LEN];//job name
    char m_sExecuteDate[MAX_TIME_LEN];//执行时间点
    int   m_iInterval;                      //执行频率间隔
    int   m_iRateType;                   //执行频率类型
    char m_sSQL[MAX_SQL_LEN];//待执行的 sql
public:
    char m_sNextExecuteDate[MAX_TIME_LEN];//计算出的下一次执行的时间
    int   m_iExcCount;                      //执行次数
    char m_sStartTime[MAX_TIME_LEN];//第一次执行的时间
private:
    int m_iState;      //当前job状态
};

class TMemSeq
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    void Init();

    char sSeqName[MAX_NAME_LEN];
    MDB_INT64 iStart;
    MDB_INT64 iEnd;
    MDB_INT64 iCur;
    MDB_INT64 iStep;
    TMutex tMutex;    //Pop的共享锁
};

//同步区类别
enum E_SYNC_AREA_TYPE
{
    SA_ORACLE    = 0,  //oracle同步区
    SA_REP          = 1,  //主备同步区
    SA_CAPTURE = 2,  //路由捕获同步区
    SA_REDO       = 3, //redo 日志同步区
    SA_SHARD_BACKUP = 4, // 分片备份同步区
    SA_MAX         
};


//同步区信息
class TMdbSyncArea
{
public:
    void Clear();//清理
    void SetFileAndSize(const char * sFile,int iSize,int iSyncType);//设置文件和大小
public:
    int    m_iSAType;//同步区类型
    char m_sName[MAX_NAME_LEN];//同步区类型
    SHAMEM_T m_iShmID;
    int m_iShmSize;          //主备同步Shm的大小(单位为M)
    MDB_INT64 m_iShmKey;           //主备同步Key
    char m_sDir[SA_MAX][MAX_PATH_NAME_LEN];  //落地的日志文件位置
    int    m_iFileSize;                 //日志文件大小，单位为M
};

//数据库的整体信息
class TMdbDSN
{
    friend class TMdbShmDSN;
    friend class TMdbVarcharMgr;
public:
    void Clear();
    void Show(bool bIfMore=false);
    long long GetLSN(){if(m_iLSN<0){m_iLSN = 0;}return m_iLSN++;}
    long long GetCurrentLSN(){if(m_iLSN<0){m_iLSN = 0;}return m_iLSN;}
    void SetLsn(long long iLsn){m_iLSN = iLsn;}
    char sVersion[MAX_NAME_LEN]; //版本号
    char sName[MAX_NAME_LEN];    //名称
    char cState;                 //状态
    //int  iDsnValue;              //dsn值
    long long  llDsnValue;              //hash值替换dsn值
    
    int iRepAttr;                //同步属性
    int iRoutingID;             //访问属性:0-不并发;1-并发
    int iTableCounts;            //表个数
    int iMutexCounts;            //锁个数, 目前暂固定为10000。
    
    int iLogLevel;               //管理进程的日志级别

    char sCreateTime[MAX_TIME_LEN]; //创建时间
    char sCurTime[MAX_TIME_LEN];    //当前时间
    struct timeval tCurTime;                 //当前时间

    //因为ShmID会随着系统的变化而变化，而Key是唯一的
    //数据段共享内存
    int iShmCounts;           
    SHAMEM_T iShmID[MAX_SHM_ID];
    long long iShmKey[MAX_SHM_ID];  

    //变长存储区共享内存数
    int iVarCharShmCounts;
    SHAMEM_T iVarCharShmID[MAX_VARCHAR_SHM_ID];
    long long iVarCharShmKey[MAX_VARCHAR_SHM_ID];  //因为ShmID会随着系统的变化而变化，而Key是唯一的

    //基础索引段
    int iBaseIndexShmCounts;          
    SHAMEM_T iBaseIndexShmID[MAX_SHM_ID];
    int iBaseIndexShmKey[MAX_SHM_ID]; //因为ShmID会随着系统的变化而变化，而Key是唯一的
    
    //冲突索引段
    int iConflictIndexShmCounts;          //冲突索引的个数
    SHAMEM_T iConflictIndexShmID[MAX_SHM_ID];
    int iConflictIndexShmKey[MAX_SHM_ID]; //因为ShmID会随着系统的变化而变化，而Key是唯一的

    TMdbSyncArea m_arrSyncArea;//同步区

    // MHASH  索引段
    int iMHashBaseIdxShmCnt;  // 阶梯式索引基础索引段个数
    SHAMEM_T iMHashBaseIdxShmID[MAX_SHM_ID]; // 阶梯式索引基础索引段shmid
    int iMHashBaseIdxShmKey[MAX_SHM_ID]; // 阶梯式索引基础索引段shmkey

    // MHASH  锁段
    int iMHashMutexShmCnt;  // 阶梯式索引基础索引段个数
    SHAMEM_T iMHashMutexShmID[MAX_SHM_ID]; // 阶梯式索引基础索引段shmid
    int iMHashMutexShmKey[MAX_SHM_ID]; // 阶梯式索引基础索引段shmkey

    SHAMEM_T iMhashConfMgrShmId; // 阶梯式索引冲突索引管理区shmid
    int iMhashConfMgrShmKey; // 阶梯式索引冲突索引管理区shmkey

    int iMHashConfIdxShmCnt;  // 阶梯式索引冲突索引段个数
    SHAMEM_T iMHashConfIdxShmID[MAX_MHASH_SHMID_COUNT]; // 阶梯式索引冲突索引段shmid
    int iMHashConfIdxShmKey[MAX_MHASH_SHMID_COUNT]; // 阶梯式索引冲突索引段shmkey

    SHAMEM_T iMhashLayerMgrShmId; // 阶梯式索引冲突索引管理区shmid
    int iMhashLayerMgrShmKey; // 阶梯式索引冲突索引管理区shmkey

    int iMHashLayerIdxShmCnt;  // 阶梯式索引阶梯索引段个数
    SHAMEM_T iMHashLayerIdxShmID[MAX_MHASH_SHMID_COUNT]; // 阶梯式索引阶梯索引段shmid
    int iMHashLayerIdxShmKey[MAX_MHASH_SHMID_COUNT];   // 阶梯式索引阶梯索引段shmkey


	//Trie 树Root索引段
    int iTrieRootIdxShmCnt;  
    SHAMEM_T iTrieRootIdxShmID[MAX_BRIE_SHMID_COUNT]; 
    int iTrieRootIdxShmKey[MAX_BRIE_SHMID_COUNT]; 
	

	//Trie 树分支索引段
	SHAMEM_T iTrieBranchMgrShmId; 
    int iTrieBranchMgrShmKey;

    int iTrieBranchIdxShmCnt;  
    SHAMEM_T iTrieBranchIdxShmID[MAX_BRIE_SHMID_COUNT]; 
    int iTrieBranchIdxShmKey[MAX_BRIE_SHMID_COUNT]; 

	//Trie 冲突索引段
    SHAMEM_T iTrieConfMgrShmId; // 阶梯式索引冲突索引管理区shmid
    int iTrieConfMgrShmKey; // 阶梯式索引冲突索引管理区shmkey

    int iTrieConfIdxShmCnt;  // 阶梯式索引阶梯索引段个数
    SHAMEM_T iTrieConfIdxShmID[MAX_BRIE_SHMID_COUNT]; // 阶梯式索引阶梯索引段shmid
    int iTrieConfIdxShmKey[MAX_BRIE_SHMID_COUNT];   // 阶梯式索引阶梯索引段shmkey

	

 
    char sStorageDir[MAX_PATH_NAME_LEN];//文件存储位置
    char sDataStore[MAX_PATH_NAME_LEN]; //文件影像的位置
    int  iPermSize;                     //只有在表信息不确定的情况下，才需要设定

    char sOracleID[MAX_NAME_LEN];       //对应的OracleID
    char sOracleUID[MAX_NAME_LEN];      //对应的OracleUID
    char sOraclePWD[MAX_NAME_LEN];      //对应的OraclePWD
    //--HH--char cType;

    char sLocalIP[MAX_IP_LEN];   //本地IP
    char sPeerIP[MAX_IP_LEN];    //对端IP
    int  iLocalPort;             //本地端口
    int  iPeerPort;              //对端端口
    int  iAgentPort[MAX_AGENT_PORT_COUNTS];             //本地代理端口
    int  iNtcPort[MAX_AGENT_PORT_COUNTS]; 
	int  iNoNtcPort[MAX_AGENT_PORT_COUNTS];
    size_t m_iDataSize;
    TMutex tMutex;            //管理区共享锁

    char m_sRepFileName[MAX_PATH_NAME_LEN];
    long m_iRepFileDataPos;
    char m_sRepSecondFileName[MAX_PATH_NAME_LEN];
    long m_iRepSecondFileDataPos;
    bool bDiskSpaceStat;//磁盘剩余空间是否够
    bool m_bIsOraRep;  //是否进行oracle同步
    bool m_bIsRep; //是否进行主备同步
    bool m_bIsCaptureRouter;// 是否捕获路由
    bool m_bIsDiskStorage;//是否开启磁盘存储   
    int m_arrRouterToCapture[MAX_ROUTER_LIST_LEN];//等待caputre的路由链表
    bool m_bDiscDisasterLink;//用于主机恢复备机链接时主机需要断开发送LocalFileLog目录文件给容灾的链接
    bool m_bLoadFromDisk; //是否从磁盘加载
    bool m_bLoadFromDiskOK;//从磁盘加载是否成功
public:
    size_t iPageMutexAddr;       //页锁管理区偏移量
    size_t iVarcharPageMutexAddr;//varchar页锁管理区偏移量
    int m_iTimeDifferent;//时差
    int m_iOraRepCounts;        //oracle备份进程数
private:
    size_t iProcAddr;            //进程信息的偏移量

    size_t iUserTableAddr;       //用户管理区偏移量
    size_t iTableSpaceAddr;      //用户表空间偏移量
    size_t iLocalLinkAddr;            //链接管理区偏移量
    size_t iRemoteLinkAddr;            //链接管理区偏移量
    size_t iSeqAddr;             //序列管理区偏移量
    size_t iSQLAddr;             //系统SQL管理区偏移量
    size_t iObserveAddr;     //观测点管理区偏移量
    size_t iVarcharAddr;         //变长存储区
    size_t iRepLinkAddr;         //主备同步链接区
    size_t iJobAddr;//job的地址
    size_t iMHashConfAddr; //  阶梯式hash索引冲突索引管理区地址
    size_t iMHashLayerAddr; // 阶梯式hash索引阶梯索引管理区地址

	size_t iTrieConfAddr; //  树形索引冲突索引管理区地址
    size_t iTrieBranchAddr; // 树形索引索引分支节点索引管理区地址
    
    size_t iPortLinkAddr;            //链接管理区偏移量
    long long m_iLSN;// 日志序列号,每个操作【增删改】递增
    
private:
	TMutex m_SessionMutex;
	unsigned short m_iSessionID;
	
public:
	unsigned int GetSessionID();
		

};


/**
 * @brief mdb表定义
 * 
 */
class TMdbTable
{
public:
    TMdbTable()
    {
        m_iTableId = -1;
        iLoadType = 1;
        m_cZipTimeType = 'N';
        iTableLevel = TAB_TINY;
        iTabLevelCnts = 10000;
    }
    ~TMdbTable() {}
    void Clear();
    void Print();
    int Init(TMdbTable * pSrcTable);//初始化
    void Show(bool bIfMore=false);
    bool HaveNullableColumn()
    {
        return iOneRecordNullOffset <= 0?false:true;   //是否有NULL able的列
    }
    bool IsValidTable()
    {
        return (sTableName[0] != '\0');
    }
    void ResetRecordCounts()
    {//至少10000
           iRecordCounts = (iRecordCounts/10000 + 1)*10000;
           iRecordCounts +=1369;//偏移一定量,打散hash
           iTabLevelCnts = iRecordCounts;
    }
    //获取时间列长度
    int GetTimeValueLength()
    {
        switch(m_cZipTimeType)
        {
            case 'Y':
            case 'y':
                return sizeof(int);
            case 'L':
            case 'l':
                return sizeof(long long);
            case 'N':
            case 'n':
                return DATE_TIME_SIZE;
            default:
                return DATE_TIME_SIZE;
        }
        return DATE_TIME_SIZE;
    }
    int GetInnerTableLevel(const char* psTabLevel);
	TMdbColumn * GetColumnByName(const char* sColumnName);
    int GetFreePageRequestCount();

    int m_iTableId;
    char sTableName[MAX_NAME_LEN];   //表名
    char sCreateTime[MAX_TIME_LEN];  //创建时间
    char sUpdateTime[MAX_TIME_LEN];  //表结构变更
    char m_sTableSpace[MAX_NAME_LEN]; // 所属表空间名称

	int iVersion;
    int iTableLevel;
    int iTabLevelCnts;  // 表的级别对应的记录数
    int  iRecordCounts;              //表中的记录数-一个准确的估算值可以大幅度提高系统性能，默认为1万
    int  iExpandRecords;             //一次扩张的冲突节点数
    bool bReadLock;                  //是否需要加读锁-对于并非频繁插入/删除数据的表,不加锁会提高速度
    bool bWriteLock;                 //是否需要加写锁-对于Session类似的表，有可能做成单进程访问的，无需加锁
    bool bFixOffset;                 //列是否固定偏移量
    bool bFixedLength;               //表是否是定长的结构,(都是定长结构了varchar,blob 放到变长存储区中)//不带有vachar 和blob类型
    bool bIsView;                     //是否是试图
    bool bIsSysTab;  //  是否是系统表

    char cState;           //表状态
    bool bRollBack;        //是否需要回退功能，如果是ocs_session表，则不需要，如果是CustCache之类的只读表，也不需要
    //bool m_bIsZipTime; //是否需要压缩
    char m_cZipTimeType;//时间压缩方式 Y-全压缩方式(int 存储) N-非压缩方式(15字节存储)  L-长压缩方式(long long 方式存储)
    bool bIsCheckPriKey; //是否需要主键校验
    bool bIsNeedLoadFromOra;//是否需要从Oracle上载数据
    int iLoadType;//表加载方式:取值0/1，0:查询时将所有字段以~组装成一个字符传，然后再解析，1:分类型解析每个字段，如果表
    //的int行字段过多则使用0方式，如果int型字段少则使用1方式

    int  iRepAttr;            //列的同步属性:0-从Ora同步;1-向Ora同步;2-向备机同步;3-向Ora和备机同步，默认为0

    bool bIsPerfStat;


    int iColumnCounts;                    //列的个数
    TMdbColumn tColumn[MAX_COLUMN_COUNTS]; //列信息

    int iIndexCounts;                   //索引个数
    TMdbIndex tIndex[MAX_INDEX_COUNTS]; //索引信息
    TMdbPrimaryKey m_tPriKey;           //主键信息


    char m_sFilterSQL[MAX_SQL_LEN];    //过滤SQL
    char m_sLoadSQL[MAX_SQL_LEN*4];    //复杂SQL的加载
    char m_sFlushSQL[MAX_SQL_LEN*4];   //复杂SQL的刷新
    char sViewSQL[MAX_SQL_LEN*4];      //视图SQL

    TMdbMTableAttr m_tableAttr[MAX_COLUMN_COUNTS];
    int iParameterCount;//参数个数
    TMdbParameter tParameter[MAX_INDEX_COUNTS];

    long long lTotalCollIndexNodeCounts;     //全部的冲突索引节点数
    long long lLeftCollIndexNodeCounts;      //空闲的冲突索引节点数
    TMdbIndexNode tCollIndexNode;       //第一个空闲的冲突索引节点

    int iCounts;         //记录数
    int iCINCounts;      //冲突索引数
    int iFullPages;      //已满页数
    int iFullPageID;     //已满页的第一个页
    int iFreePages;      //自由页数
    int iFreePageID;     //自由页的第一个页
    TMutex tFreeMutex;   //自由页的共享锁
    TMutex tFullMutex;   //满页的共享锁
    TMutex tTableMutex;  //表信息的共享锁
    TMdbHashAlgo m_ColoumHash;
    int iOneRecordSize;  //一条记录的大小(每条记录都是定长的)数据大小+索引信息大小
    //int iOneRecordDataSize;//一条记录中的数据大小
    int iOneRecordNullOffset;//一条记录的NULL值偏移量
    int m_iTimeStampOffset; // 一条记录的时间戳偏移量

    int iInsertCounts;   //insert记录数
    int iDeleteCounts;//delete记录数
    int iUpdateCounts;//update记录数
    int iQueryCounts;//查询记录数
    int iQueryFailCounts;//查询失败记录数
    int iInsertFailCounts; //插入失败记录数
    int iDeleteFailCounts; //删除失败记录数
    int iUpdateFailCounts; //更新失败记录数
    bool m_bShardBack; // 是否分片备份
    char m_cStorageType; // 存储数据源类型
	int iMagic_n;//记录表结构变化情况
};

class TMdbMemQueue
{
public:
    int GetFreeSpace();//获取剩余空间
public:
    char sFlag[8];
    TMutex tPushMutex;   //Push的共享锁
    TMutex tPopMutex;    //Pop的共享锁
    int iPushPos;    //Push的位置
    int iPopPos;     //Pop的位置
    int iStartPos;   //数据区的起始位置
    int iEndPos;     //数据区的结束位置
    int iTailPos;    //数据的结束位置
};
	//LCR中的列信息
    class TLCRColm
    {
        public:
            TLCRColm();
			~TLCRColm();
            void Clear();
        public:
            bool m_bNull; // 是否为空
            int m_iType;
            std::string m_sColmName; // 列名
            std::string m_sColmValue; // 列值
    };
	
	//所有同步记录格式及redolog记录格式
	class TMdbLCR
	{
	public:
		TMdbLCR();
		~TMdbLCR();
		void Clear();
		int GetSelectSQL(TMdbTable* pTable);
		int GetSQL(TMdbTable* pTable);
		int GetInsertSQL(TMdbTable* pTable, int reptNum, char * sSQL);
		int GetUpdateSQL(TMdbTable* pTable, int reptNum, char * sSQL);
		int GetDeleteSQL(TMdbTable* pTable, int reptNum, char * sSQL);
		int GetSQL(TMdbTable* pTable, int reptNum, char* sSQL);
	public:
        int m_iRoutID;  // 路由ID
        int m_iSqlType; // SQL类型: insert/update/delete
        int m_iLen; // redo log 记录长度
        int m_iVersion; // 版本
        int m_iSyncFlag; // TTL
        int m_iColmLen; // 列名串长度
        long long m_iTimeStamp;    // 记录时间戳
        long long m_lLsn;   // redo log的 LSN
        long long m_lRowId; // rowid
        std::string m_sTableName;    // 记录的表名
        std::vector<TLCRColm> m_vColms; // 各列信息
        std::vector<TLCRColm> m_vWColms; // 条件列信息
        char m_sSQL[MAX_SQL_LEN]; //对于有需要拼写sql的就使用
        char m_sSelSQL[MAX_SQL_LEN]; //对于有需要反查sql的就使用
    };

    class TMdbDAONode
    {
        public:
            TMdbDAONode();
            ~TMdbDAONode();
            void Clear();
            TMdbQuery *CreateDBQuery(char* sSQL,TMdbDatabase* pDatabase,int iFlag);
            char m_sSQL[MAX_SQL_LEN]; 
            TMdbQuery * m_pQuery;
    };

    class TMdbFlushDao:public TMdbNtcBaseObject
    {
        public:
            TMdbFlushDao();
            ~TMdbFlushDao();
            TMdbQuery *CreateDBQuery(int iSqlType,char* sSQL,TMdbDatabase* pDatabase,int iFlag);
        private:
            TMdbDAONode * m_arrDaoNode[MAX_FLUSN_DAO_COUNTS];
    };

    //内存数据库执行引擎，在内存中大量执行增删改查，均可使用该接口
    class TMdbFlushEngine
    {
        public:
        TMdbFlushEngine();
        ~TMdbFlushEngine();
        int Init(char* sDsn);
        TMdbQuery *CreateDBQuery(int iSqlType,const char* sTableName,char* sSQL,int iFlag);
        int Excute(TMdbLCR & tMdbLcr,int iFlag, int * iDropIndex, bool bIsInvalidUpdate);
        int GetSQL(char* sSQL,TMdbLCR & tMdbLcr, int * iDropIndex, bool bIsInvalidUpdate);
        int SetParameter(TMdbQuery* pMdbQuery,TMdbLCR & tMdbLcr, int * iDropIndex);
        private:
        TMdbNtcStrMap m_tMapDao;
        TMdbDatabase* m_pDatabase;
    };

	//
	class TMdbIndexBlock
	{
	public:
        TMdbIndexBlock(){Clear();}
        virtual ~TMdbIndexBlock(){}
        void Clear()
        {
            iBlockId = -1;
            iNextBlock = -1;
            iStartNode = 0;
            iEndNode = 0;
            iShmID = INITVAl;
            iSize = 0;
            
        }

        int iBlockId;
        int iNextBlock;
        int iStartNode;
        int iEndNode;
        TMutex tMutex;
        SHAMEM_T iShmID;   //所属共享内存ID
        
        size_t iSize; // 共享内存块大小
	};

    // 共享内存Mhash索引节点块
    class TMdbMhashBlock:public TMdbIndexBlock
    {
    public:
        TMdbMhashBlock(){Clear();}
        ~TMdbMhashBlock(){}
    };

	// 共享内存trie索引节点块
    class TMdbTrieBlock:public TMdbIndexBlock
    {
    public:
        TMdbTrieBlock(){Clear();}
        ~TMdbTrieBlock(){}
    };
	
//}


#endif
