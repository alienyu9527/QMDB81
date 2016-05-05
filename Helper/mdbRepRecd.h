#ifndef _MDB_REP_LOG_H_
#define _MDB_REP_LOG_H_

#include <string>
#include <vector>

#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"

//namespace QuickMDB{

    
    #define NULL_VALUE_LEN (-999)

    // 同步记录头信息
    class TRepHeadInfo
    {
        public:
            TRepHeadInfo();
            ~TRepHeadInfo();

            void Clear();
            void Print();
            int GetRoutingId();
            TRepHeadInfo& operator=(const TRepHeadInfo& tHeadInfo);

        public:
            
            int m_iRoutID;  // 路由ID
            int m_iSqlType; // SQL类型: insert/update/delete
            int m_iLen; // redo log 记录长度
            int m_iVersion; // 版本
            int m_iSyncFlag; // 同步标识 
            int m_iColmLen; // 列名串长度
            long long m_iTimeStamp;    // 记录时间戳
            long long m_lLsn;   // redo log的 LSN
            long long m_lRowId; // rowid
            char m_sTableName[MAX_NAME_LEN];    // 记录的表名
            //std::vector<std::string> m_vColmName; // 列名
            
    };

    // 同步记录列信息
    class TRepColm
    {
        public:
            TRepColm();
            ~TRepColm();

            void Clear();
            void Print();

        public:
            bool m_bNull; // 是否为空
            std::string m_sColmName; // 列名
            std::string m_sColmValue; // 列值
    };

    // MDB 数据同步记录
    class TMdbRepRecd
    {
    public:
        TMdbRepRecd();
        ~TMdbRepRecd();

        void Clear();
        void Print();

    public:
        TRepHeadInfo m_tHeadInfo; // 记录头信息
        std::vector<TRepColm> m_vColms; // 各列信息
        std::vector<TRepColm> m_vWColms; // 条件列信息
    };

    // 1.2版本主备同步格式解析
    class TMdbRep12Decode
    {
    public:
        TMdbRep12Decode();
        ~TMdbRep12Decode();
        
        int Analyse(const  char * pDataStr, TMdbConfig* pMdbConfig,TMdbRepRecd &tRecd); // 按1.2版本格式解析
        
    private:

        int GetNum(const  char* pNum,int iNumSize);//获取数值型
        int GetColumnValue(const char* pValuePair,TRepColm & stColumn,int &iValuePairLen, bool& bPush);//获取列值
        int GetData(const char * pData,std::vector<TRepColm> & vData,int &iLen);
        void GetNullPos(const char * pData);
        
    private:

        char m_sTemp[MAX_VALUE_LEN];
        TMdbTable * m_pCurTab;
        int iFactor[10];
        int m_NullColPos[MAX_COLUMN_COUNTS];
    };

    class TMdbRepRecdDecode 
    {
    public:
        TMdbRepRecdDecode();
        ~TMdbRepRecdDecode();

        int GetHeadInfo(const char* psData, TRepHeadInfo& tHeadInfo, int& iHeadLen);
        int DeSerialize(const  char * psData,TMdbRepRecd &tRecd);
        int GetRoutingID(const char* psData);
        char GetVersion(const char* psData);
        

    private:
        int GetNum(const  char* pNum,int iNumSize);
        int GetColmName(const char* psData,TMdbRepRecd &tRecd, int & iColmCnt);

    private :
        int m_iFactor[10]; // 解析数值型用
        char m_sDataBuff[MAX_VALUE_LEN]; // 临时缓存
        //std::vector<TRepColm> m_vColm;
        //std::vector<TRepColm> m_vWColm;
        bool m_bWhere;
    };

    class TMdbRepRecdEnCode
    {
    public:
        TMdbRepRecdEnCode();
        ~TMdbRepRecdEnCode();

        void Clear();
        int Serialize(char * pDataBuff,int iBuffLen, int &iRecdLen);
        void AddHeadInfo(const TRepHeadInfo& tHeadInfo);
        void AddColm(std::string sName, std::string sValue, bool bNull = false);
        void AddWhereColm(std::string sName, std::string sValue, bool bNull = false);
        void SetColmCnt(const int iColmCnt);

    private:

        // 清理各种临时缓存
        void ClearBuff();

    private:
        int m_iColumCnt;
        TMdbRepRecd m_tRecd; // 保存需要编码的记录信息
        char m_sNameBuff[MAX_VALUE_LEN];
        char m_sColmLenBuff[MAX_VALUE_LEN];
        char m_sColmValueBuff[MAX_VALUE_LEN];
    };

    
//}

#endif
