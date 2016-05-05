/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbFileParser.h		
*@Description: 解析同步文件，逐条获取记录
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_FILE_PARSER_H__
#define __ZTE_MINI_DATABASE_REP_FILE_PARSER_H__

#include "Control/mdbRepCommon.h"
#include "Helper/mdbRepRecd.h"
//namespace QuickMDB
//{
    //一个文件记录解析出来的数据结果
    class TMdbOneRepRecord
    {
    public:
        TMdbOneRepRecord();
        ~TMdbOneRepRecord();
        void Clean();
        char m_sData[MAX_VALUE_LEN];
        int m_iLen;
    };


    //处理一个文件,拆分出一条条的记录，并进行错误记录和记录规整 
    class TMdbRepFileParser
    {
    public:
        TMdbRepFileParser();
        ~TMdbRepFileParser();

    public:
        //设置读取的目录
        int Init(const char* pszFullFileName);

        //处理下一个记录
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
        //获取文件大小
        long GetFileSize(const char* pszFullFileName);

    private:
        char *m_pMemBuffer;   //一个文件的缓冲
        long m_iMemSize;               //缓冲大小
        FILE* m_fp;                    //文件句柄
        long m_iPos;                   //当前读取的位置
        long m_iFileSize;              //当前文件大小
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