/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbDictionary.h		
*@Description�� mdb�ֵ䶨��
*@Author:		jin.shaohua
*@Date��	    2014/02/28
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
#define MAX_MDB_COLUMN 100 //mdb�������
#define MAX_MDB_INDEX 10    //mdb�����������
#define MAX_BIT_MAP_SIZE (10240) //bitmap��С��λ���ֽ�
#define MAX_BLOB_LEN (8192)   //blob��������
#define MAX_SQL_LEN (8192)   //SQL��󳤶�
#define MAX_INDEX_COLUMN_NAME  (128)  //���������б���
#define MAX_TABLESPACE_COUNT (256) //����ռ����
#define MAX_FLUSH_CHANGED_PAGE_COUNT (100) //���һ��д����ʱ�ļ�����ҳ����

#define MAX_MDB_ROWID_SIZE  (sizeof(int))   //rowid��С(page-id + dataoffset)��4 �ֽ�
#define MAX_MDB_PAGE_COUNT    (2097151)      //ÿ����ռ����ҳ������ռrowid��21λ
#define MAX_MDB_PAGE_RECORD_COUNT (2047) //ҳ������¼����ռrowid��11λ
#define MAX_VALUE_LEN      (32768)  //������󳤶�
#define TS_FILE_PREFIX "TS_"      //�ļ�ǰ׺
#define VARCHAR_FILE_PREFIX "VARCHAR_"      //�ļ�ǰ׺
#define TS_FILE_SUFFIX ".mdb"   //�ļ���׺
#define TS_CHANGE_FILE_SUFFIX ".change"   //��ʱ�ļ���׺
#define BAK_FILE_SUFFIX ".bak"   //��ʱ�ļ���׺
//QMDB֧�ֵ���������������:
#define DT_Unknown   0 //δ֪����
#define DT_Int       1 //����д��Int(4)��Int(8)��Int(11)��������ʽ��Ĭ����Int(8)
#define DT_Char      2 //�ַ��ͣ��̶����ȣ�����Char(2)��ʾ��2���ַ���ϵͳ�ڲ����Զ�����
#define DT_VarChar   3 //�䳤��������	
#define DT_DateStamp 4 //�������ͣ���ʽΪ��YYYYMMDDHHM24SS��
#define DT_Blob      9 //BLOB���ͣ������Ʊ�ʾ
#define MAX_INDEX_COUNTS  10

// ����ʽ�������ײ���
#define MAX_INDEX_LAYER_LIMIT 4

#define MAX_BASE_INDEX_COUNTS 500

#define MAX_MHASH_INDEX_COUNT 1000
#define MAX_MHASH_SHMID_COUNT 1000

#define MAX_BRIE_INDEX_COUNT  100
#define MAX_BRIE_SHMID_COUNT 100

#define MAX_TRIE_WORD_LEN    256


//VARCHAR ֧�ֵĶ�
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
	 * @brief rowid һ����¼��Ψһ��ʶ
	 * 
	 */
	class TMdbRowID
        {
        public:
            void Clear()
            {
                m_iRowID = 0;
            }
            //��ȡҳ��
            int GetPageID();
            //��ȡ����ƫ��
            int GetDataOffset();
            //����pageid
            int SetPageID(int iPageID);
            int SetDataOffset(int iDataOffset);
			int SetRowId(unsigned int iRowId){m_iRowID = iRowId;return 0;}
            bool IsEmpty(){return 0==m_iRowID;}
            bool operator==(const TMdbRowID &t1)const{
                 return (this->m_iRowID == t1.m_iRowID);
             }  
			unsigned int m_iRowID;
        private:
            
           // int iPageID;  //����ҳ��
           // int iDataOffset;     //��ҳ�е�����ƫ��
        };


	enum DATA_FLAG
	{
		DATA_REAL = 0,
		DATA_VIRTUAL = 0x01,
		DATA_DELETE = 0x02,
		DATA_RECYCLE = 0x04
	};
	/**
	 * @brief ҳ�ڵ�
	 * 
	 */
	class TMdbPageNode
	{
	public:
		void Print(){printf("SessionID:%d,cFlag:%d\n",iSessionID,cFlag);}
	public:
		char   cStorage;  //������¼�Ƿ����ļ��洢�ģ���Ҫ����varchar����varcharҳ��ʹ�ø��ֶ� Y:�� N:����
		int    iNextNode; //��һ���ڵ㣬�������������Դ��δʹ�õĽڵ� <0 ��ʾ�ýڵ�����ʹ�ã�>=0 ��ʾ�ýڵ����
		int    iPreNode; //ǰһ���ڵ�
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
			void Init(int iPageID, int iPageSize, int iVarcharID);//��ʼ��varcharҳ
            int GetFreeRecord(int & iNewDataOffset,int iDataSize);//��ȡһ�����еļ�¼�ռ�
            int PushBack(int iDataOffset);//�黹һ����¼�ռ�
            std::string ToString();//���
            //�Ƿ��ǺϷ���dataOffset
            bool IsValidDataOffset(int iDataOffset)
            {
                return true == (iDataOffset < m_iFreeOffSet && iDataOffset >= (int)sizeof(TMdbPage) + (int)sizeof(TMdbPageNode));
            }
            bool bNeedToMoveToFreeList(){return (m_iFreeSize * 4 >= m_iPageSize)?true:false;};//�Ƿ����
            char * GetNextDataAddr(char * &pCurDataAddr,int & iDataOffset,char *&pNextDataAddr);//��ȡ��һ�����ݵ�ַ
            //��¼λ��תdata offset
            int RecordPosToDataOffset(int iRecordPos);
            //data offset ת��¼λ��
            int DataOffsetToRecordPos(int iDataOffset);
			int LockRow(int iDataOffset,char* pHeadAddr);
			int UnLockRow(int iDataOffset,char* pHeadAddr);
			
        public:
            char m_sUpdateTime[MAX_TIME_LEN];    //���ʱ��
            int m_iPageID;          //ҳ��
            int m_iPrePageID;       //��һ��ҳ��, ���һ��˫���������Խ��з�����ƶ�
            int m_iNextPageID;      //��һ��ҳ��
            int m_iRecordCounts;    //�ܼ�¼��
            int m_iPageSize;        //ҳ��С,Ĭ��Ϊ4096
            int m_iFreePageNode; //��һ�����е�ҳ�ڵ�
            int m_iFullPageNode;//���һ�����ڵ㣬Ҳ���ǰ������ݵĽڵ㴮������
            int m_iFreeOffSet;   //����ƫ����
            int m_iRecordSize;  //���ݵĳ���[���ݳ����Ƕ�����]
            char m_sState[8];     //״̬��full-����; free-����; empty-��; move -ҳ��Ǩ��
            int m_iFreeSize;      //���пռ�
            char m_sTableName[MAX_NAME_LEN]; //��ǰҳ�����ĸ���
            //int m_iTableID;//��ǰ�����ĸ���ID
            long long m_iPageLSN;//ҳ������
        };


	//Table-Space �ڵ�
	class TMdbTSNode
        {
        public:
            TMdbTSNode();
            void Clear();
            int iPageStart;  //��ʼҳ��
            int iPageEnd;    //����ҳ��
            SHAMEM_T iShmID;   //���������ڴ�ID
            size_t iOffSet;  //��ʼҳ�ڹ����ڴ��е�ƫ����
            size_t iNext;       //��һ���ڵ�ƫ����
            char bmDirtyPage[MAX_BIT_MAP_SIZE];//��ҳ��,bitmap
        };


	/**
	 * @brief ��ռ�
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
            bool m_bFileStorage; // �Ƿ����ļ��洢�ı�ռ�
            int m_iBlockId; //�ļ��洢���
            int m_iChangeBlockId; //�ļ��洢��ʱ�ļ����
            char sName[MAX_NAME_LEN];      //��ռ�����
            size_t  iPageSize;       //ҳ��С��Ĭ��Ϊ4096
            int  iRequestCounts;  //һ�������ҳ������Ĭ��Ϊ1��
            int  iEmptyPages;     //����ҳ��
            int  iTotalPages;     //�ܵ�ҳ��
            TMdbTSNode tNode;     //�ڵ���Ϣ
            int  iEmptyPageID;    //����ҳ�ĵ�һ��ҳ
            TMutex tEmptyMutex;   //����ҳ�Ĺ�����
            TMutex m_tFreeMutex; // ����ҳ�Ĺ�����
            TMutex m_tFullMutex; // ��ҳ�Ĺ�����
            char cState;      //��ռ�״̬����0��-δ����;��1��-�Ѿ�����;��2��-���ڴ���;��3����������
            char sCreateTime[MAX_TIME_LEN]; //��ռ䴴��ʱ��
        };

	/*varchar ������*/
	class TMdbVarchar
	{
	public:
		TMdbVarchar();
		~TMdbVarchar();
		void Clear();
		int iVarcharID;   //��ʾ���ĸ��εģ�16/32/64/128��
		int iBlockId;      //�ļ��洢��ţ�1���ļ���СΪ1G
		int iChangeBlockId; 
		size_t iPageSize; //ҳ���С��ҳ���С���ݶ���ȷ��
		int iTotalPages;     //��ҳ��
		int iFreePageId; //������
		int iFullPageId; //����
		TMdbTSNode tNode;     //ͷ�ڵ�
		TMutex tMutex;   //����ҳ�Ĺ�����
	};
	/**
	 * @brief mdb���ж���
	 * 
	 */

	class TMdbColumn
        {
        public:
            void Clear();
            void Print();
            bool bIncrementalUpdate();//�Ƿ�������������
            bool IsStrDataType();//�Ƿ����ַ������͵��ֶ�
            char sName[MAX_NAME_LEN]; //������
            int  iDataType;           //�е���������
            int  iColumnLen; 	      //�е����ݳ��ȣ�����Ǳ䳤������Ϊ-1
            int  iPos;                //�е�λ��, ��0...n
            int  iOffSet;             //���ݵ�ƫ����
            bool isInOra;             //Oracle���Ƿ���ڱ��ֶ�,Ĭ����true���������ֶ���session��ģ���Ϊfalse
            long iHashPos;            //�ڵ���hash���е�λ��
            bool bIsDefault;
            char iDefaultValue[256];
            bool m_bNullable;//�Ƿ�����null;
        };
	/**
	 * @brief ��������
	 * 
	 */
	enum E_MDB_INDEX_TYPE
	{
		INDEX_HASH = 0,     //hash����
		INDEX_M_HASH    = 1,     //multistep hash index
		INDEX_TRIE = 2,     //trie����
		INDEX_UNKOWN 
	};

	// ��ļ�¼������
	enum E_MDB_TABLE_LEVEL
	{
		TAB_TINY = 1, // 1w����
		TAB_MINI =2, // 50w
		TAB_SMALL =3, // 200w
		TAB_LARGE = 4, // 1000w
		TAB_HUGE = 5, // 5000w
		TAB_ENORMOUS // 10000w
	};

	/**
	 * @brief ������Ϣ
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

            char sName[MAX_NAME_LEN]; //��������
            int m_iAlgoType;  // �������㷨����: hash , multistep hash, btree
            int m_iIndexType; // ����������: int , char, composite index
            int iColumnNo[MAX_INDEX_COLUMN_COUNTS];       //�����������е����кţ���0��ʼ��
            int iPriority;       //�������ȼ�,1��10����ֵԽС���ȼ�Խ��
            //TMutex tMutex;       //�����Ĺ�����
            int iMaxLayer;
			bool bBuilding;
        };


    //��������
class TMdbPrimaryKey
{
public:
    void Clear();
    void Print();

    int iColumnCounts;                 //������Ҫ������
    int iColumnNo[MAX_PRIMARY_KEY]; //������Ӧ���к�
    char m_sTableName[MAX_NAME_LEN] ;
};

    class TMdbMTableAttr
{
public:
    void Clear();

    char sAttrName[MAX_NAME_LEN];   //������
    char sAttrValue[1024];        //����ֵ
    bool bIsExist;                 //�����Ƿ����
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



    //�䳤���ݵĴ洢��
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
        int  iShmNo;   //�����ڴ�����кţ����ﲢ��ʹ��ID,Ŀ��Ϊ�˿��ٷ���
        int  iOffSet;   //�ڹ����ڴ��е�ƫ����
        int  iSize;     //�ڹ����ڴ��еĴ�С����λ��M
        //bool m_bIsAttach;
        bool m_bState;  //�Ƿ��Ѿ�����.
    };

    //�����ڴ���ͷ�ṹ, �������ڷǹ������ͷǻ�����������
    class TMdbShmHead
    {
    public:
        SHAMEM_T iShmID;  //�������ڴ�ε�ID
        size_t iTotalSize;  //�ܴ�С
        size_t iLeftOffSet; //ʣ��ռ�ƫ����
        size_t iAdvOffSet;  //�����ڴ�ƫ��

        int iDataLen;    //���ڱ䳤�洢��ͷ��Ϣ,��¼�洢�����ĸ����ȵ�.
        int iRecordCounts; //��ǰ�䳤�洢�����ŵļ�¼��.
        int iTotalRecords; //�ñ䳤�洢��һ���ܴ�ŵļ�¼��.

        //vector<long> vfreeList; //��¼���еļ�¼��λ��
        int iFirstFreeRecord;    //��¼��ǰ��Ŀ��еļ�¼��λ��

        TMutex tMutex;    //������������

        //ȡ��һ���ڴ�iSize-�����С��iReal-ʵ�ʴ�С��iPageSize-ҳ���С��iPageCounts-����ҳ����
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
        char sName[128];                            //��¼���������ƣ�������ȷ��У�顣
        int  iShmID[MAX_VARCHAR_SHM_ID];           //��¼10000�������ڴ�飬ͨ�����㹻�ˡ�
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
        
        int iOffSet;                     //��һ����ڵ��λ��

        //һ��1000����,��ǰʹ�õ��ĸ���.
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

    //�����ڵ�
    class TMdbIndexNode
    {
        public:
        TMdbRowID tData;  //��������λ��
        int       iNextPos;//��һ��node ��λ��
        // int 	iPrePos;//ǰһ��node��λ��
    };

    // �����ڴ���ϵĿ��п飬�����ڴ����
    class TMDBIndexFreeSpace
    {
    public:
        void Clear()
        {
            iPosAdd = 0;
            iSize   = 0;
        }
        size_t iPosAdd;  //���п�ƫ����
        size_t iSize;	  //���п��С
    };
    //����������Ϣ
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
        char sName[MAX_NAME_LEN];   //��������
        char sTabName[MAX_NAME_LEN]; // ����
        size_t  iPosAdd;               //����ƫ����
        size_t  iSize;                 //�������������С����λΪ�ֽ�
        char cState;                //����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������;
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iConflictMgrPos;         //��ͻ��������pos
        int  iConflictIndexPos;     //��ͻ����pos
    };


    //����������������Ϣ
    class TMdbBaseIndexMgrInfo
    {
    public:
        int iSeq;         //�ڼ��������ڴ��
        TMutex tMutex;    //������������
        TMdbBaseIndex tIndex[MAX_BASE_INDEX_COUNTS]; //����������Ϣ
        int iIndexCounts;  //����������
        TMDBIndexFreeSpace tFreeSpace[MAX_BASE_INDEX_COUNTS];//���пռ�
        MDB_INT64 iTotalSize;   //�ܴ�С
    };

    //��ͻ������Ϣ
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
        size_t GetTotalCount(){return iSize/sizeof(TMdbIndexNode);}//�ܸ���
        size_t iPosAdd;               //����ƫ����
        size_t  iSize;                 //���������С����λΪ�ֽ�
        char cState;                //����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iFreeHeadPos;         //����ͷ���λ��
        int  iFreeNodeCounts;      //ʣ����нڵ���
    };

    //��ͻ������������Ϣ
    class TMdbConflictIndexMgrInfo
    {
    public:
        int iSeq;         //�ڼ��������ڴ��
        TMutex tMutex;    //������������
        TMdbConflictIndex tIndex[MAX_BASE_INDEX_COUNTS]; //����������Ϣ
        int iIndexCounts;  //����������
        TMDBIndexFreeSpace tFreeSpace[MAX_BASE_INDEX_COUNTS];//���пռ�
        MDB_INT64 iTotalSize;   //�ܴ�С
    };

    

    //mdbÿ����¼��ÿ�������ڵ��λ��
    class TMdbRowIndex
    {
    public:
        void Clear(){iBaseIndexPos = -1;iConflictIndexPos = -1;iPreIndexPos = -1;}
        bool IsPreNodeInConflict(){return iPreIndexPos >= 0?true:false;}   //ǰ�ýڵ��Ƿ��ڳ�ͻ����
        bool IsCurNodeInConflict(){return iConflictIndexPos >= 0?true:false;}//��ǰ�ڵ��Ƿ��ڳ�ͻ����
    public:
        int iBaseIndexPos;     //���������ڵ�λ��
        int iConflictIndexPos;//��ͻ�����ڵ�λ�� < 0��ʾ���ڳ�ͻ����
        int iPreIndexPos;       //ǰһ���ڵ�λ��     < 0 ��ʾ���ڳ�ͻ����
    };

    
//���ݿ��еĵĽ�����Ϣ
class TMdbProc
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    bool IsValid(){return (0 != sName[0] && 0 != sStartTime[0]);}
public:
    char sName[MAX_NAME_LEN];      //��������
    char cState;                   //����״̬:'S'-ֹͣ; 'F'-����; 'B'-æµ
    int  iPid;                     //������ϵͳ�е�PID
    char sStartTime[MAX_TIME_LEN]; //��������ʱ��
    char sStopTime[MAX_TIME_LEN];  //����ֹͣʱ��
    char sUpdateTime[MAX_TIME_LEN]; //���̸���ʱ��
    int  iLogLevel;                 //���̵���־����
    bool bIsMonitor;                //�Ƿ���Ҫ���
    int Serialize(MDB_JSON_WRITER_DEF & writer);//���л�
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

//���ݿ�Զ��������Ϣ
class TMdbRemoteLink
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    bool IsValid(){return ( 0 != sIP[0] && 0 != sStartTime[0]);}
    bool IsCurrentThreadLink();//�Ƿ��ǵ�ǰ�̵߳�����
public:
    char sIP[MAX_IP_LEN]; //���̵�IP
    int iHandle;            //���ӵľ��
    char cState;          //����״̬
    char sStartTime[MAX_TIME_LEN]; //��������ʱ��
    char sFreshTime[MAX_TIME_LEN]; //�����������ʱ�䣬�൱������
    int  iLogLevel;                //���ӵ���־����
    char cAccess;                  //Ȩ��, A-����Ա;W-�ɶ�д;R-ֻ��
    char sUser[MAX_NAME_LEN];      //�û���
    char sPass[MAX_NAME_LEN];      //����
    char sDSN[MAX_NAME_LEN];       //DSN����
    int iLowPriority;              //���ȼ�
    int iPID;                      //���̵�PID
    unsigned long iTID; 			//Զ�����ӵ��߳�ID
    int iSQLPos;                    //Զ��������sqlλ��
    int iProtocol;                  //Э��
    char sProcessName[MAX_NAME_LEN];   //�ĸ����̴�������
};

class TMdbCSLink
{//CS ����ע����Ϣ
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
    int iFD; //�ͻ���socket����ID
    struct sockaddr_in tAddr;
    TMdbRemoteLink * m_pRemoteLink;
    char *m_sUser;
    char *m_sPass;
    char *m_sDSN;
    char m_sClientIP[MAX_IP_LEN];
    int m_iClientPID;//�ͻ���PID
    int m_iClientTID;//�ͻ����߳�ID
    int m_iLowPriority;//���ȼ�
    int m_iUseOcp;//Э��    
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

//job����,�꣬�£��գ�Сʱ������,��
enum E_JOB_RATE_TYPE
{
    JOB_PER_NONE =0,//��ЧƵ��
    JOB_PER_YEAR = 1,
    JOB_PER_MONTH,
    JOB_PER_DAY,
    JOB_PER_HOUR,
    JOB_PER_MIN,
    JOB_PER_SEC
};


//job״̬
enum E_JOB_STATE
{
   JOB_STATE_NONE = 1,//��Ч״̬
   JOB_STATE_WAIT ,    //�ȴ�ִ��
   JOB_STATE_RUNNING //����ִ��
};

class TMdbJob
{
public:
    TMdbJob();
    int Clear();//����
    int SetRateType(const char * sRateType);//����RateType
    int GetRateType(char*pRateType,const int iLen);//��ȡRateType
    std::string ToString();//��ӡ
    bool IsValid(){return JOB_PER_NONE != m_iRateType && 0 != m_sSQL[0] && 0 != m_iInterval;}//�Ƿ�����Ч��job
    void StateToWait(){m_iState = JOB_STATE_WAIT;}//���״̬���ȴ�̬
    void StateToRunning(){m_iState = JOB_STATE_RUNNING;};//���״̬��ִ��̬
    bool IsCanModify(){return JOB_STATE_RUNNING != m_iState;};//�Ƿ���Ա����
public:
    char m_sName[MAX_NAME_LEN];//job name
    char m_sExecuteDate[MAX_TIME_LEN];//ִ��ʱ���
    int   m_iInterval;                      //ִ��Ƶ�ʼ��
    int   m_iRateType;                   //ִ��Ƶ������
    char m_sSQL[MAX_SQL_LEN];//��ִ�е� sql
public:
    char m_sNextExecuteDate[MAX_TIME_LEN];//���������һ��ִ�е�ʱ��
    int   m_iExcCount;                      //ִ�д���
    char m_sStartTime[MAX_TIME_LEN];//��һ��ִ�е�ʱ��
private:
    int m_iState;      //��ǰjob״̬
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
    TMutex tMutex;    //Pop�Ĺ�����
};

//ͬ�������
enum E_SYNC_AREA_TYPE
{
    SA_ORACLE    = 0,  //oracleͬ����
    SA_REP          = 1,  //����ͬ����
    SA_CAPTURE = 2,  //·�ɲ���ͬ����
    SA_REDO       = 3, //redo ��־ͬ����
    SA_SHARD_BACKUP = 4, // ��Ƭ����ͬ����
    SA_MAX         
};


//ͬ������Ϣ
class TMdbSyncArea
{
public:
    void Clear();//����
    void SetFileAndSize(const char * sFile,int iSize,int iSyncType);//�����ļ��ʹ�С
public:
    int    m_iSAType;//ͬ��������
    char m_sName[MAX_NAME_LEN];//ͬ��������
    SHAMEM_T m_iShmID;
    int m_iShmSize;          //����ͬ��Shm�Ĵ�С(��λΪM)
    MDB_INT64 m_iShmKey;           //����ͬ��Key
    char m_sDir[SA_MAX][MAX_PATH_NAME_LEN];  //��ص���־�ļ�λ��
    int    m_iFileSize;                 //��־�ļ���С����λΪM
};

//���ݿ��������Ϣ
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
    char sVersion[MAX_NAME_LEN]; //�汾��
    char sName[MAX_NAME_LEN];    //����
    char cState;                 //״̬
    //int  iDsnValue;              //dsnֵ
    long long  llDsnValue;              //hashֵ�滻dsnֵ
    
    int iRepAttr;                //ͬ������
    int iRoutingID;             //��������:0-������;1-����
    int iTableCounts;            //�����
    int iMutexCounts;            //������, Ŀǰ�ݹ̶�Ϊ10000��
    
    int iLogLevel;               //������̵���־����

    char sCreateTime[MAX_TIME_LEN]; //����ʱ��
    char sCurTime[MAX_TIME_LEN];    //��ǰʱ��
    struct timeval tCurTime;                 //��ǰʱ��

    //��ΪShmID������ϵͳ�ı仯���仯����Key��Ψһ��
    //���ݶι����ڴ�
    int iShmCounts;           
    SHAMEM_T iShmID[MAX_SHM_ID];
    long long iShmKey[MAX_SHM_ID];  

    //�䳤�洢�������ڴ���
    int iVarCharShmCounts;
    SHAMEM_T iVarCharShmID[MAX_VARCHAR_SHM_ID];
    long long iVarCharShmKey[MAX_VARCHAR_SHM_ID];  //��ΪShmID������ϵͳ�ı仯���仯����Key��Ψһ��

    //����������
    int iBaseIndexShmCounts;          
    SHAMEM_T iBaseIndexShmID[MAX_SHM_ID];
    int iBaseIndexShmKey[MAX_SHM_ID]; //��ΪShmID������ϵͳ�ı仯���仯����Key��Ψһ��
    
    //��ͻ������
    int iConflictIndexShmCounts;          //��ͻ�����ĸ���
    SHAMEM_T iConflictIndexShmID[MAX_SHM_ID];
    int iConflictIndexShmKey[MAX_SHM_ID]; //��ΪShmID������ϵͳ�ı仯���仯����Key��Ψһ��

    TMdbSyncArea m_arrSyncArea;//ͬ����

    // MHASH  ������
    int iMHashBaseIdxShmCnt;  // ����ʽ�������������θ���
    SHAMEM_T iMHashBaseIdxShmID[MAX_SHM_ID]; // ����ʽ��������������shmid
    int iMHashBaseIdxShmKey[MAX_SHM_ID]; // ����ʽ��������������shmkey

    // MHASH  ����
    int iMHashMutexShmCnt;  // ����ʽ�������������θ���
    SHAMEM_T iMHashMutexShmID[MAX_SHM_ID]; // ����ʽ��������������shmid
    int iMHashMutexShmKey[MAX_SHM_ID]; // ����ʽ��������������shmkey

    SHAMEM_T iMhashConfMgrShmId; // ����ʽ������ͻ����������shmid
    int iMhashConfMgrShmKey; // ����ʽ������ͻ����������shmkey

    int iMHashConfIdxShmCnt;  // ����ʽ������ͻ�����θ���
    SHAMEM_T iMHashConfIdxShmID[MAX_MHASH_SHMID_COUNT]; // ����ʽ������ͻ������shmid
    int iMHashConfIdxShmKey[MAX_MHASH_SHMID_COUNT]; // ����ʽ������ͻ������shmkey

    SHAMEM_T iMhashLayerMgrShmId; // ����ʽ������ͻ����������shmid
    int iMhashLayerMgrShmKey; // ����ʽ������ͻ����������shmkey

    int iMHashLayerIdxShmCnt;  // ����ʽ�������������θ���
    SHAMEM_T iMHashLayerIdxShmID[MAX_MHASH_SHMID_COUNT]; // ����ʽ��������������shmid
    int iMHashLayerIdxShmKey[MAX_MHASH_SHMID_COUNT];   // ����ʽ��������������shmkey


	//Trie ��Root������
    int iTrieRootIdxShmCnt;  
    SHAMEM_T iTrieRootIdxShmID[MAX_BRIE_SHMID_COUNT]; 
    int iTrieRootIdxShmKey[MAX_BRIE_SHMID_COUNT]; 
	

	//Trie ����֧������
	SHAMEM_T iTrieBranchMgrShmId; 
    int iTrieBranchMgrShmKey;

    int iTrieBranchIdxShmCnt;  
    SHAMEM_T iTrieBranchIdxShmID[MAX_BRIE_SHMID_COUNT]; 
    int iTrieBranchIdxShmKey[MAX_BRIE_SHMID_COUNT]; 

	//Trie ��ͻ������
    SHAMEM_T iTrieConfMgrShmId; // ����ʽ������ͻ����������shmid
    int iTrieConfMgrShmKey; // ����ʽ������ͻ����������shmkey

    int iTrieConfIdxShmCnt;  // ����ʽ�������������θ���
    SHAMEM_T iTrieConfIdxShmID[MAX_BRIE_SHMID_COUNT]; // ����ʽ��������������shmid
    int iTrieConfIdxShmKey[MAX_BRIE_SHMID_COUNT];   // ����ʽ��������������shmkey

	

 
    char sStorageDir[MAX_PATH_NAME_LEN];//�ļ��洢λ��
    char sDataStore[MAX_PATH_NAME_LEN]; //�ļ�Ӱ���λ��
    int  iPermSize;                     //ֻ���ڱ���Ϣ��ȷ��������£�����Ҫ�趨

    char sOracleID[MAX_NAME_LEN];       //��Ӧ��OracleID
    char sOracleUID[MAX_NAME_LEN];      //��Ӧ��OracleUID
    char sOraclePWD[MAX_NAME_LEN];      //��Ӧ��OraclePWD
    //--HH--char cType;

    char sLocalIP[MAX_IP_LEN];   //����IP
    char sPeerIP[MAX_IP_LEN];    //�Զ�IP
    int  iLocalPort;             //���ض˿�
    int  iPeerPort;              //�Զ˶˿�
    int  iAgentPort[MAX_AGENT_PORT_COUNTS];             //���ش���˿�
    int  iNtcPort[MAX_AGENT_PORT_COUNTS]; 
	int  iNoNtcPort[MAX_AGENT_PORT_COUNTS];
    size_t m_iDataSize;
    TMutex tMutex;            //������������

    char m_sRepFileName[MAX_PATH_NAME_LEN];
    long m_iRepFileDataPos;
    char m_sRepSecondFileName[MAX_PATH_NAME_LEN];
    long m_iRepSecondFileDataPos;
    bool bDiskSpaceStat;//����ʣ��ռ��Ƿ�
    bool m_bIsOraRep;  //�Ƿ����oracleͬ��
    bool m_bIsRep; //�Ƿ��������ͬ��
    bool m_bIsCaptureRouter;// �Ƿ񲶻�·��
    bool m_bIsDiskStorage;//�Ƿ������̴洢   
    int m_arrRouterToCapture[MAX_ROUTER_LIST_LEN];//�ȴ�caputre��·������
    bool m_bDiscDisasterLink;//���������ָ���������ʱ������Ҫ�Ͽ�����LocalFileLogĿ¼�ļ������ֵ�����
    bool m_bLoadFromDisk; //�Ƿ�Ӵ��̼���
    bool m_bLoadFromDiskOK;//�Ӵ��̼����Ƿ�ɹ�
public:
    size_t iPageMutexAddr;       //ҳ��������ƫ����
    size_t iVarcharPageMutexAddr;//varcharҳ��������ƫ����
    int m_iTimeDifferent;//ʱ��
    int m_iOraRepCounts;        //oracle���ݽ�����
private:
    size_t iProcAddr;            //������Ϣ��ƫ����

    size_t iUserTableAddr;       //�û�������ƫ����
    size_t iTableSpaceAddr;      //�û���ռ�ƫ����
    size_t iLocalLinkAddr;            //���ӹ�����ƫ����
    size_t iRemoteLinkAddr;            //���ӹ�����ƫ����
    size_t iSeqAddr;             //���й�����ƫ����
    size_t iSQLAddr;             //ϵͳSQL������ƫ����
    size_t iObserveAddr;     //�۲�������ƫ����
    size_t iVarcharAddr;         //�䳤�洢��
    size_t iRepLinkAddr;         //����ͬ��������
    size_t iJobAddr;//job�ĵ�ַ
    size_t iMHashConfAddr; //  ����ʽhash������ͻ������������ַ
    size_t iMHashLayerAddr; // ����ʽhash��������������������ַ

	size_t iTrieConfAddr; //  ����������ͻ������������ַ
    size_t iTrieBranchAddr; // ��������������֧�ڵ�������������ַ
    
    size_t iPortLinkAddr;            //���ӹ�����ƫ����
    long long m_iLSN;// ��־���к�,ÿ����������ɾ�ġ�����
    
private:
	TMutex m_SessionMutex;
	unsigned short m_iSessionID;
	
public:
	unsigned int GetSessionID();
		

};


/**
 * @brief mdb����
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
    int Init(TMdbTable * pSrcTable);//��ʼ��
    void Show(bool bIfMore=false);
    bool HaveNullableColumn()
    {
        return iOneRecordNullOffset <= 0?false:true;   //�Ƿ���NULL able����
    }
    bool IsValidTable()
    {
        return (sTableName[0] != '\0');
    }
    void ResetRecordCounts()
    {//����10000
           iRecordCounts = (iRecordCounts/10000 + 1)*10000;
           iRecordCounts +=1369;//ƫ��һ����,��ɢhash
           iTabLevelCnts = iRecordCounts;
    }
    //��ȡʱ���г���
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
    char sTableName[MAX_NAME_LEN];   //����
    char sCreateTime[MAX_TIME_LEN];  //����ʱ��
    char sUpdateTime[MAX_TIME_LEN];  //��ṹ���
    char m_sTableSpace[MAX_NAME_LEN]; // ������ռ�����

	int iVersion;
    int iTableLevel;
    int iTabLevelCnts;  // ��ļ����Ӧ�ļ�¼��
    int  iRecordCounts;              //���еļ�¼��-һ��׼ȷ�Ĺ���ֵ���Դ�������ϵͳ���ܣ�Ĭ��Ϊ1��
    int  iExpandRecords;             //һ�����ŵĳ�ͻ�ڵ���
    bool bReadLock;                  //�Ƿ���Ҫ�Ӷ���-���ڲ���Ƶ������/ɾ�����ݵı�,������������ٶ�
    bool bWriteLock;                 //�Ƿ���Ҫ��д��-����Session���Ƶı��п������ɵ����̷��ʵģ��������
    bool bFixOffset;                 //���Ƿ�̶�ƫ����
    bool bFixedLength;               //���Ƿ��Ƕ����Ľṹ,(���Ƕ����ṹ��varchar,blob �ŵ��䳤�洢����)//������vachar ��blob����
    bool bIsView;                     //�Ƿ�����ͼ
    bool bIsSysTab;  //  �Ƿ���ϵͳ��

    char cState;           //��״̬
    bool bRollBack;        //�Ƿ���Ҫ���˹��ܣ������ocs_session������Ҫ�������CustCache֮���ֻ����Ҳ����Ҫ
    //bool m_bIsZipTime; //�Ƿ���Ҫѹ��
    char m_cZipTimeType;//ʱ��ѹ����ʽ Y-ȫѹ����ʽ(int �洢) N-��ѹ����ʽ(15�ֽڴ洢)  L-��ѹ����ʽ(long long ��ʽ�洢)
    bool bIsCheckPriKey; //�Ƿ���Ҫ����У��
    bool bIsNeedLoadFromOra;//�Ƿ���Ҫ��Oracle��������
    int iLoadType;//����ط�ʽ:ȡֵ0/1��0:��ѯʱ�������ֶ���~��װ��һ���ַ�����Ȼ���ٽ�����1:�����ͽ���ÿ���ֶΣ������
    //��int���ֶι�����ʹ��0��ʽ�����int���ֶ�����ʹ��1��ʽ

    int  iRepAttr;            //�е�ͬ������:0-��Oraͬ��;1-��Oraͬ��;2-�򱸻�ͬ��;3-��Ora�ͱ���ͬ����Ĭ��Ϊ0

    bool bIsPerfStat;


    int iColumnCounts;                    //�еĸ���
    TMdbColumn tColumn[MAX_COLUMN_COUNTS]; //����Ϣ

    int iIndexCounts;                   //��������
    TMdbIndex tIndex[MAX_INDEX_COUNTS]; //������Ϣ
    TMdbPrimaryKey m_tPriKey;           //������Ϣ


    char m_sFilterSQL[MAX_SQL_LEN];    //����SQL
    char m_sLoadSQL[MAX_SQL_LEN*4];    //����SQL�ļ���
    char m_sFlushSQL[MAX_SQL_LEN*4];   //����SQL��ˢ��
    char sViewSQL[MAX_SQL_LEN*4];      //��ͼSQL

    TMdbMTableAttr m_tableAttr[MAX_COLUMN_COUNTS];
    int iParameterCount;//��������
    TMdbParameter tParameter[MAX_INDEX_COUNTS];

    long long lTotalCollIndexNodeCounts;     //ȫ���ĳ�ͻ�����ڵ���
    long long lLeftCollIndexNodeCounts;      //���еĳ�ͻ�����ڵ���
    TMdbIndexNode tCollIndexNode;       //��һ�����еĳ�ͻ�����ڵ�

    int iCounts;         //��¼��
    int iCINCounts;      //��ͻ������
    int iFullPages;      //����ҳ��
    int iFullPageID;     //����ҳ�ĵ�һ��ҳ
    int iFreePages;      //����ҳ��
    int iFreePageID;     //����ҳ�ĵ�һ��ҳ
    TMutex tFreeMutex;   //����ҳ�Ĺ�����
    TMutex tFullMutex;   //��ҳ�Ĺ�����
    TMutex tTableMutex;  //����Ϣ�Ĺ�����
    TMdbHashAlgo m_ColoumHash;
    int iOneRecordSize;  //һ����¼�Ĵ�С(ÿ����¼���Ƕ�����)���ݴ�С+������Ϣ��С
    //int iOneRecordDataSize;//һ����¼�е����ݴ�С
    int iOneRecordNullOffset;//һ����¼��NULLֵƫ����
    int m_iTimeStampOffset; // һ����¼��ʱ���ƫ����

    int iInsertCounts;   //insert��¼��
    int iDeleteCounts;//delete��¼��
    int iUpdateCounts;//update��¼��
    int iQueryCounts;//��ѯ��¼��
    int iQueryFailCounts;//��ѯʧ�ܼ�¼��
    int iInsertFailCounts; //����ʧ�ܼ�¼��
    int iDeleteFailCounts; //ɾ��ʧ�ܼ�¼��
    int iUpdateFailCounts; //����ʧ�ܼ�¼��
    bool m_bShardBack; // �Ƿ��Ƭ����
    char m_cStorageType; // �洢����Դ����
	int iMagic_n;//��¼��ṹ�仯���
};

class TMdbMemQueue
{
public:
    int GetFreeSpace();//��ȡʣ��ռ�
public:
    char sFlag[8];
    TMutex tPushMutex;   //Push�Ĺ�����
    TMutex tPopMutex;    //Pop�Ĺ�����
    int iPushPos;    //Push��λ��
    int iPopPos;     //Pop��λ��
    int iStartPos;   //����������ʼλ��
    int iEndPos;     //�������Ľ���λ��
    int iTailPos;    //���ݵĽ���λ��
};
	//LCR�е�����Ϣ
    class TLCRColm
    {
        public:
            TLCRColm();
			~TLCRColm();
            void Clear();
        public:
            bool m_bNull; // �Ƿ�Ϊ��
            int m_iType;
            std::string m_sColmName; // ����
            std::string m_sColmValue; // ��ֵ
    };
	
	//����ͬ����¼��ʽ��redolog��¼��ʽ
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
        int m_iRoutID;  // ·��ID
        int m_iSqlType; // SQL����: insert/update/delete
        int m_iLen; // redo log ��¼����
        int m_iVersion; // �汾
        int m_iSyncFlag; // TTL
        int m_iColmLen; // ����������
        long long m_iTimeStamp;    // ��¼ʱ���
        long long m_lLsn;   // redo log�� LSN
        long long m_lRowId; // rowid
        std::string m_sTableName;    // ��¼�ı���
        std::vector<TLCRColm> m_vColms; // ������Ϣ
        std::vector<TLCRColm> m_vWColms; // ��������Ϣ
        char m_sSQL[MAX_SQL_LEN]; //��������Ҫƴдsql�ľ�ʹ��
        char m_sSelSQL[MAX_SQL_LEN]; //��������Ҫ����sql�ľ�ʹ��
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

    //�ڴ����ݿ�ִ�����棬���ڴ��д���ִ����ɾ�Ĳ飬����ʹ�øýӿ�
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
        SHAMEM_T iShmID;   //���������ڴ�ID
        
        size_t iSize; // �����ڴ���С
	};

    // �����ڴ�Mhash�����ڵ��
    class TMdbMhashBlock:public TMdbIndexBlock
    {
    public:
        TMdbMhashBlock(){Clear();}
        ~TMdbMhashBlock(){}
    };

	// �����ڴ�trie�����ڵ��
    class TMdbTrieBlock:public TMdbIndexBlock
    {
    public:
        TMdbTrieBlock(){Clear();}
        ~TMdbTrieBlock(){}
    };
	
//}


#endif
