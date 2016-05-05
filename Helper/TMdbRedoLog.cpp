#include "Helper/TMdbRedoLog.h"

//namespace QuickMDB{
    TRedoRecdHead::TRedoRecdHead()
    {
        Clear();
    }

    TRedoRecdHead::~TRedoRecdHead()
    {
    }

    void TRedoRecdHead::Clear()
    {
        m_iRoutID = -1;
        m_iSqlType = -1;
        m_iLen = 0;
        m_iTimeStamp = -1;
        m_lLsn = -1;
        memset(m_sTableName, 0, sizeof(m_sTableName));
    }

    int TRedoRecdHead::GetRoutingId()
    {
        return m_iRoutID;
    }

    TRecdColm::TRecdColm()
    {
        Clear();
    }

    TRecdColm::~TRecdColm()
    {
    }

    void TRecdColm::Clear()
    {
        m_bNull = false;
        m_sColmName.Clear();
        m_sColmValue.Clear();
    }

    TRedoRecd::TRedoRecd()
    {
        Clear();
    }

    TRedoRecd::~TRedoRecd()
    {
    }

    void TRedoRecd::Clear()
    {
        m_tHeadInfo.Clear();
        m_vColms.clear();
    }


    TRedoLogParser::TRedoLogParser()
    {
    }

    TRedoLogParser::~TRedoLogParser()
    {
    }

    int TRedoLogParser::ParseLogHead(const char* psData, TRedoRecdHead& tLogHead)
    {
        int iRet = 0;
        // TODO: PARSE HEAD
        return iRet;
    }

    int TRedoLogParser::Parse(const char* psData, TRedoRecd& tLogRecd)
    {
        int iRet = 0;
        // TODO: PARSE 
        return iRet;
    }

//}