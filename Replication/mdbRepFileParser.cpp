/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbFileParser.cpp		
*@Description: 解析同步文件，逐条获取记录
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#include "Replication/mdbRepFileParser.h"

//namespace QuickMDB
//{
    //处理一个文件,拆分出一条条的记录，并进行错误记录和记录规整 
    TMdbOneRepRecord::TMdbOneRepRecord()
    {
        //memset(m_sData,0,MAX_VALUE_LEN);
        m_sData[0] = '\0';
        m_iLen = 0;
    }
    TMdbOneRepRecord::~TMdbOneRepRecord()
    {
    }
    void TMdbOneRepRecord::Clean()
    {
        m_sData[0] = '\0';
        m_iLen = 0;
    }

    TMdbRepFileParser::TMdbRepFileParser()
    {
        m_pMemBuffer = NULL;  //一个文件的缓冲
        m_iMemSize   = 0;     //缓冲大小
        m_fp         = NULL;  //文件句柄
        m_iPos       = 0;     //当前读取的位置
        m_iFileSize  = 0;     //当前文件大小
        m_pOneRecord = NULL;
    }


    TMdbRepFileParser::~TMdbRepFileParser()
    {
        SAFE_DELETE(m_pMemBuffer);
        SAFE_DELETE(m_pOneRecord);
        SAFE_CLOSE(m_fp);   
    }


    //设置读取的目录
    int TMdbRepFileParser::Init(const char* pszFullFileName)
    {
        TADD_FUNC("Start. FileName = [%s]", pszFullFileName);
        int iRet = ERROR_SUCCESS;

        SAFESTRCPY(m_sFullFileName,sizeof(m_sFullFileName),pszFullFileName); 
        if(m_pOneRecord == NULL)
        {
            m_pOneRecord = new(std::nothrow) TMdbOneRepRecord();   
            CHECK_OBJ(m_pOneRecord);
        }

        //获取文件大小
        long iFileSize = GetFileSize(pszFullFileName);
        if(iFileSize < 0)
        {
            CHECK_RET(ERR_OS_GET_FILE_SIZE, "GetFileSize() failed.", pszFullFileName); 
        }

        //如果内存空间不足，则申请更大的内存
        if(iFileSize > m_iMemSize)
        {
            if(m_pMemBuffer != NULL)
            {
                SAFE_DELETE(m_pMemBuffer);
            }             

            m_pMemBuffer = new(std::nothrow) char[iFileSize+100]; 
            CHECK_OBJ(m_pMemBuffer);

            m_iMemSize = sizeof(m_pMemBuffer);
        }

        memset(m_pMemBuffer,0,m_iMemSize);

        //关闭原来的文件
        SAFE_CLOSE(m_fp);

        //打开文件
        m_fp = fopen(pszFullFileName, "rb");
        if(m_fp == NULL)
        {
            CHECK_RET(ERR_OS_OPEN_FILE, "Open file [%s] failed. errno = [%d], errmsg = [%s]", pszFullFileName, errno, strerror(errno));
        }

        //把文件内容都读入内存中
        fread(m_pMemBuffer, iFileSize, 1, m_fp);
        m_iPos      = 0;
        m_iFileSize = iFileSize;

        TADD_FUNC("Finish. FileName = [%s], FileSize = [%d] .", pszFullFileName, m_iFileSize);
        return iRet;
    }

    //处理下一个记录
    TMdbOneRepRecord* TMdbRepFileParser::Next()
    {
        TADD_FUNC("Start.");

        TADD_DETAIL("m_iPos=%ld, m_iFileSize=%ld.", m_iPos, m_iFileSize);
        m_pOneRecord->Clean();
      
        int iLen = 0;
        while (m_iPos<m_iFileSize)
        {
            if (MDB_REP_RCD_BEGIN[0] == m_pMemBuffer[m_iPos] && MDB_REP_RCD_BEGIN[1] == m_pMemBuffer[m_iPos+1] )
            {
                iLen   = (m_pMemBuffer[m_iPos+2]-'0')*1000 + (m_pMemBuffer[m_iPos+3]-'0')*100 +
                             (m_pMemBuffer[m_iPos+4]-'0')*10   + (m_pMemBuffer[m_iPos+5]-'0');
                
                if(iLen < 10 || m_pMemBuffer[m_iPos+iLen-1] != '#' || m_pMemBuffer[m_iPos+iLen-2] != '#')
                {
                    char sTemp[128] = { 0 };
                    strncpy(sTemp, (char*)&m_pMemBuffer[m_iPos], 127);
					sTemp[127] = '\0';
                    TADD_ERROR(ERROR_UNKNOWN,"Invalid data=[%s].", sTemp);
                    ++m_iPos;
                    continue;
                }

                m_pOneRecord->m_iLen =iLen;
                strncpy(m_pOneRecord->m_sData, &m_pMemBuffer[m_iPos], m_pOneRecord->m_iLen);
                m_pOneRecord->m_sData[m_pOneRecord->m_iLen] = '\0';
                m_iPos+=iLen;
                TADD_FUNC("Finish.");
                return m_pOneRecord;
                
            }
            else
            {
                m_iPos++;
            }
            
        }


        TADD_FUNC("Finish.");
        return NULL;
    }


    long TMdbRepFileParser::GetFileSize(const char* pszFullFileName)
    {
        struct stat f_stat;
        if(stat(pszFullFileName, &f_stat) == -1) 
        {
            TADD_ERROR(ERR_OS_GET_FILE_SIZE, "File stat failed. errno = [%d]", errno);
            return -1;
        }
        return (long)f_stat.st_size;
    }   

    TMdbRepFileStat::TMdbRepFileStat()
    {
        //m_tStatMap.SetAutoRelease(true);
        Clear();
    }

    TMdbRepFileStat::~TMdbRepFileStat()
    {

    }

    void TMdbRepFileStat::Clear()
    {
        //TRepTableStatInfo* pStatInfo;
        //for (int i = 0; i<m_tStatMap.GetSize(); i++)
        //{
        //    pStatInfo = m_tStatMap[i];
        //    pStatInfo->Clear();
        //}
        //m_tStatMap.Clear();
        m_tStatMap.clear();
    }

    /******************************************************************************
    * 函数名称	:  Stat
    * 函数描述	:  统计同步文件中的每张表的操作情况
    * 输入		:  pOneRepRecord 同步记录
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbRepFileStat::Stat(TMdbOneRepRecord *pOneRepRecord)
    {
        if(NULL == pOneRepRecord)
        {
            return;
        }
        TRepHeadInfo tHead;
        int iLen = 0;
        m_tRcdParser.GetHeadInfo((char*)pOneRepRecord, tHead,iLen);
        m_iSQLType = tHead.m_iSqlType;
       
        if (m_tStatMap.find(tHead.m_sTableName) == m_tStatMap.end())
        {
            TRepTableStatInfo tStatInfo;
            tStatInfo.Clear();
            tStatInfo.m_strTableName = tHead.m_sTableName;
            m_tStatMap.insert(std::pair<std::string, TRepTableStatInfo>(tStatInfo.m_strTableName, tStatInfo));
        }

        TRepTableStatInfo *pStatInfo = &m_tStatMap.find(tHead.m_sTableName)->second;
  
        switch(m_iSQLType)
        {
        case TK_INSERT:
            pStatInfo->m_iInsertCount++;
            break;
        case TK_DELETE:
            pStatInfo->m_iDeleteCount++;
            break;
        case TK_UPDATE:
            pStatInfo->m_iUpdateCount++;
            break;
        default:
            break;
        }
        return;
    }

    /******************************************************************************
    * 函数名称	:  PrintStatInfo
    * 函数描述	:  单元每个同步文件中每张表的操作情况
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbRepFileStat::PrintStatInfo()
    {
        TRepTableStatInfo *pStatInfo;
        std::map<std::string, TRepTableStatInfo>::iterator itor = m_tStatMap.begin();
        for(; itor!=m_tStatMap.end(); ++itor)
        {
            pStatInfo = &itor->second;
            TADD_NORMAL("Statistics: TableName = [%s],Insert = [%d],Update = [%d],Delete = [%d].",\
                pStatInfo->m_strTableName.c_str(), pStatInfo->m_iInsertCount, pStatInfo->m_iUpdateCount, pStatInfo->m_iDeleteCount);
        }
    }


    int TMdbRepFileStat::GetSqlType()
    {
        return m_iSQLType;
    }
//}