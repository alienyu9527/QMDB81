#ifndef _MDB_REDO_LOG_H_
#define _MDB_REDO_LOG_H_

#include "Helper/mdbStruct.h"
//#include "BillingSDK.h"
#include <vector>

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    /**
    * @brief redo log 记录中的关键头信息
    * 
    */
    class TRedoRecdHead
    {
        public:
            TRedoRecdHead();
            ~TRedoRecdHead();

            /**
            * @brief 重置信息
            * 
            */
            void Clear();

            /**
            * @brief  获取routing_id
            * 
            * @return int
            */
            int GetRoutingId();

        public:
            
            int m_iRoutID;  // 路由ID
            int m_iSqlType; // SQL类型: insert/update/delete
            int m_iLen; // redo log 记录长度
            time_t m_iTimeStamp;    // 记录时间戳
            long long m_lLsn;   // redo log的 LSN
            char m_sTableName[MAX_NAME_LEN];    // 记录的表名
            
    };

    /**
    * @brief 保存解析后redo log 中一列的信息
    * 
    */
    class TRecdColm
    {
        public:
            TRecdColm();
            ~TRecdColm();

            void Clear();

        public:
            bool m_bNull; // 是否为空
            TString m_sColmName; // 列名
            TString m_sColmValue; // 列值
    };

    /**
    * @brief redo log 记录信息
    * 
    */
    class TRedoRecd
    {
        public:
            TRedoRecd();
            ~TRedoRecd();

            /**
            * @brief 重置信息
            * 
            */
            void Clear();

        public:
            TRedoRecdHead m_tHeadInfo; // 记录头信息
            std::vector<TRecdColm> m_vColms; // 各列信息
    };

    /**
    * @brief redo log 解析类
    * 
    */
    class TRedoLogParser
    {
        public:
            TRedoLogParser();
            ~TRedoLogParser();

            /**
            * @brief 解析redolog 的头信息
            * 
            * @param psData [in] redolog 记录
            * @param tLogHead [out] redolog 头信息
            * @return int
            * @retval 成功为0，失败为错误码
            */
            int ParseLogHead(const char* psData, TRedoRecdHead& tLogHead);

            /**
            * @brief 解析redolog的完整信息
            * 
            * @param psData [in] redolog 记录
            * @param  tLogRecd [out] redolog 信息
            * @return int
            * @retval  成功为0，失败为错误码
            */
            int Parse(const char* psData, TRedoRecd& tLogRecd);
};


//}

#endif