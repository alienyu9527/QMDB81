#ifndef _MDB_REDO_LOG_H_
#define _MDB_REDO_LOG_H_

#include "Helper/mdbStruct.h"
//#include "BillingSDK.h"
#include <vector>

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    /**
    * @brief redo log ��¼�еĹؼ�ͷ��Ϣ
    * 
    */
    class TRedoRecdHead
    {
        public:
            TRedoRecdHead();
            ~TRedoRecdHead();

            /**
            * @brief ������Ϣ
            * 
            */
            void Clear();

            /**
            * @brief  ��ȡrouting_id
            * 
            * @return int
            */
            int GetRoutingId();

        public:
            
            int m_iRoutID;  // ·��ID
            int m_iSqlType; // SQL����: insert/update/delete
            int m_iLen; // redo log ��¼����
            time_t m_iTimeStamp;    // ��¼ʱ���
            long long m_lLsn;   // redo log�� LSN
            char m_sTableName[MAX_NAME_LEN];    // ��¼�ı���
            
    };

    /**
    * @brief ���������redo log ��һ�е���Ϣ
    * 
    */
    class TRecdColm
    {
        public:
            TRecdColm();
            ~TRecdColm();

            void Clear();

        public:
            bool m_bNull; // �Ƿ�Ϊ��
            TString m_sColmName; // ����
            TString m_sColmValue; // ��ֵ
    };

    /**
    * @brief redo log ��¼��Ϣ
    * 
    */
    class TRedoRecd
    {
        public:
            TRedoRecd();
            ~TRedoRecd();

            /**
            * @brief ������Ϣ
            * 
            */
            void Clear();

        public:
            TRedoRecdHead m_tHeadInfo; // ��¼ͷ��Ϣ
            std::vector<TRecdColm> m_vColms; // ������Ϣ
    };

    /**
    * @brief redo log ������
    * 
    */
    class TRedoLogParser
    {
        public:
            TRedoLogParser();
            ~TRedoLogParser();

            /**
            * @brief ����redolog ��ͷ��Ϣ
            * 
            * @param psData [in] redolog ��¼
            * @param tLogHead [out] redolog ͷ��Ϣ
            * @return int
            * @retval �ɹ�Ϊ0��ʧ��Ϊ������
            */
            int ParseLogHead(const char* psData, TRedoRecdHead& tLogHead);

            /**
            * @brief ����redolog��������Ϣ
            * 
            * @param psData [in] redolog ��¼
            * @param  tLogRecd [out] redolog ��Ϣ
            * @return int
            * @retval  �ɹ�Ϊ0��ʧ��Ϊ������
            */
            int Parse(const char* psData, TRedoRecd& tLogRecd);
};


//}

#endif