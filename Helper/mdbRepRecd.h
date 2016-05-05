#ifndef _MDB_REP_LOG_H_
#define _MDB_REP_LOG_H_

#include <string>
#include <vector>

#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"

//namespace QuickMDB{

    
    #define NULL_VALUE_LEN (-999)

    // ͬ����¼ͷ��Ϣ
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
            
            int m_iRoutID;  // ·��ID
            int m_iSqlType; // SQL����: insert/update/delete
            int m_iLen; // redo log ��¼����
            int m_iVersion; // �汾
            int m_iSyncFlag; // ͬ����ʶ 
            int m_iColmLen; // ����������
            long long m_iTimeStamp;    // ��¼ʱ���
            long long m_lLsn;   // redo log�� LSN
            long long m_lRowId; // rowid
            char m_sTableName[MAX_NAME_LEN];    // ��¼�ı���
            //std::vector<std::string> m_vColmName; // ����
            
    };

    // ͬ����¼����Ϣ
    class TRepColm
    {
        public:
            TRepColm();
            ~TRepColm();

            void Clear();
            void Print();

        public:
            bool m_bNull; // �Ƿ�Ϊ��
            std::string m_sColmName; // ����
            std::string m_sColmValue; // ��ֵ
    };

    // MDB ����ͬ����¼
    class TMdbRepRecd
    {
    public:
        TMdbRepRecd();
        ~TMdbRepRecd();

        void Clear();
        void Print();

    public:
        TRepHeadInfo m_tHeadInfo; // ��¼ͷ��Ϣ
        std::vector<TRepColm> m_vColms; // ������Ϣ
        std::vector<TRepColm> m_vWColms; // ��������Ϣ
    };

    // 1.2�汾����ͬ����ʽ����
    class TMdbRep12Decode
    {
    public:
        TMdbRep12Decode();
        ~TMdbRep12Decode();
        
        int Analyse(const  char * pDataStr, TMdbConfig* pMdbConfig,TMdbRepRecd &tRecd); // ��1.2�汾��ʽ����
        
    private:

        int GetNum(const  char* pNum,int iNumSize);//��ȡ��ֵ��
        int GetColumnValue(const char* pValuePair,TRepColm & stColumn,int &iValuePairLen, bool& bPush);//��ȡ��ֵ
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
        int m_iFactor[10]; // ������ֵ����
        char m_sDataBuff[MAX_VALUE_LEN]; // ��ʱ����
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

        // ���������ʱ����
        void ClearBuff();

    private:
        int m_iColumCnt;
        TMdbRepRecd m_tRecd; // ������Ҫ����ļ�¼��Ϣ
        char m_sNameBuff[MAX_VALUE_LEN];
        char m_sColmLenBuff[MAX_VALUE_LEN];
        char m_sColmValueBuff[MAX_VALUE_LEN];
    };

    
//}

#endif
