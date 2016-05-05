#include "Helper/mdbConfigAlter.h"


//namespace QuickMDB{

    TIntAlterAttr::TIntAlterAttr()
    {
        Clear();
    }

    TIntAlterAttr::~TIntAlterAttr()
    {
    }

    void TIntAlterAttr::Clear()
    {
        m_iOldValue = -1;
        m_iNewValue = -1;
    }

    TStringAlterAttr::TStringAlterAttr()
    {
        Clear();
    }

    TStringAlterAttr::~TStringAlterAttr()
    {
    }

    void TStringAlterAttr::Clear()
    {
        m_sNewValue.clear();
        m_sOldValue.clear();
    }

    TMdbTsAlterInfo::TMdbTsAlterInfo()
    {
        Clear();
    }

    TMdbTsAlterInfo::~TMdbTsAlterInfo()
    {
    }

    void TMdbTsAlterInfo::Clear()
    {
        m_vAddTs.clear();
        m_vDelTs.clear();
        m_vPageSizeAlter.clear();
    }

    bool TMdbTsAlterInfo::HaveAnyChange()
    {
        if(m_vAddTs.size() > 0 || m_vDelTs.size() > 0 || m_vPageSizeAlter.size() > 0)
        {
            return true;
        }
        return false;
    }

    TColumnAlter::TColumnAlter()
    {
        Clear();
    }

    TColumnAlter::~TColumnAlter()
    {
    }

    void TColumnAlter::Clear()
    {
        m_sColmName.clear();
        m_bLenAlter = false;
        m_bTypeAlter = false;
        m_tAlterInfo.Clear();
        //m_tDataLen.Clear();
        //m_tDataType.Clear();
    }
	
	TColumnInfo::TColumnInfo()
	{
		Clear();
	}

	TColumnInfo::~TColumnInfo()
	{
	}

	void TColumnInfo::Clear()
	{
		m_sName.clear();
		m_iPos = -1;
	}

    TMdbTabAlterInfo::TMdbTabAlterInfo()
    {
        Clear();
    }

    TMdbTabAlterInfo::~TMdbTabAlterInfo()
    {
    }

    void TMdbTabAlterInfo::Clear()
    {
        m_sTableName.clear();
        m_bTsAlter = false;
        m_bZipTime = false;
        m_tTsAlter.Clear();
        m_vColumnAlter.clear();
        m_vAddColumn.clear();
		m_vDropColumn.clear();
    }

    bool TMdbTabAlterInfo::HaveAnyChange()
    {
        if(m_bTsAlter || m_bZipTime||m_vColumnAlter.size() > 0 || m_vAddColumn.size() > 0 || m_vDropColumn.size() > 0)
        {
            return true;
        }

        return false;
    }



//}

