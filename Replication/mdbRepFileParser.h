/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbFileParser.h		
*@Description: ����ͬ���ļ���������ȡ��¼
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_FILE_PARSER_H__
#define __ZTE_MINI_DATABASE_REP_FILE_PARSER_H__

#include "Control/mdbRepCommon.h"
#include "Helper/mdbRepRecd.h"
//namespace QuickMDB
//{
    //һ���ļ���¼�������������ݽ��
    class TMdbOneRepRecord
    {
    public:
        TMdbOneRepRecord();
        ~TMdbOneRepRecord();
        void Clean();
        char m_sData[MAX_VALUE_LEN];
        int m_iLen;
    };


    //����һ���ļ�,��ֳ�һ�����ļ�¼�������д����¼�ͼ�¼���� 
    class TMdbRepFileParser
    {
    public:
        TMdbRepFileParser();
        ~TMdbRepFileParser();

    public:
        //���ö�ȡ��Ŀ¼
        int Init(const char* pszFullFileName);

        //������һ����¼
        TMdbOneRepRecord* Next(); 

        long GetFilePos()
        {
            return m_iPos;
        }

        void SetFilePos(long iPos)
        {
            m_iPos = iPos;
        }

    private:
        //��ȡ�ļ���С
        long GetFileSize(const char* pszFullFileName);

    private:
        char *m_pMemBuffer;   //һ���ļ��Ļ���
        long m_iMemSize;               //�����С
        FILE* m_fp;                    //�ļ����
        long m_iPos;                   //��ǰ��ȡ��λ��
        long m_iFileSize;              //��ǰ�ļ���С
        TMdbOneRepRecord *m_pOneRecord;
        char m_sFullFileName[MAX_PATH_NAME_LEN];
    };

    struct TRepTableStatInfo/*:public TMdbNtcBaseObject*/
    {
        void Clear()
        {
            m_iInsertCount = 0;
            m_iDeleteCount = 0;
            m_iUpdateCount = 0;
        }
        std::string m_strTableName;
        int m_iInsertCount;
        int m_iUpdateCount;
        int m_iDeleteCount;
    };

    class TMdbRepFileStat
    {
    public:
        TMdbRepFileStat();
        ~TMdbRepFileStat();

        void Clear();
        void Stat(TMdbOneRepRecord *pOneRepRecord);
        void PrintStatInfo();
        int GetSqlType();
    private:
        int GetTableID(TMdbOneRepRecord *pOneRepRecord);
        int GetSqlType(TMdbOneRepRecord *pOneRepRecord);
    private:
        TMdbRepRecdDecode m_tRcdParser;
        int m_iSQLType;
        std::map<std::string, TRepTableStatInfo>m_tStatMap;
    };

//}

#endif