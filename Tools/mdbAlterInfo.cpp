/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbAterInfo.cpp       
*@Description�� �����޸��ڴ����ݿ�ϵͳ������Ϣ
*@Author:       zhang.lin
*@Date��        2012��02��23��
*@History:
******************************************************************************************/
#include "Tools/mdbAlterInfo.h"
#include "Interface/mdbQuery.h"
#include "Common/mdbStrUtils.h"
#include "Control/mdbScript.h"

//namespace QuickMDB{

    TMdbAlterInfo::TMdbAlterInfo()
    {
        m_pConfig = NULL; 
        m_pShmDSN = NULL;
        m_pDsn    = NULL;     
        m_pMdbTable = NULL;
        m_pMdbTableSpace = NULL;
        m_pColumn = NULL;
        m_sAttrName[0] = 0;
    }


    TMdbAlterInfo::~TMdbAlterInfo()
    {
        
    }

    /******************************************************************************
    * ��������  :  LinkDsn()
    * ��������  :  ����ĳ��DSN�����ǲ��ڹ�����ע���κ���Ϣ    
    * ����      :  pszDSN, ��������������DSN 
    * ���      :  ��
    * ����ֵ    :  �ɹ�����true, ʧ�ܷ���false
    * ����      :  zhang.lin
    *******************************************************************************/
    bool TMdbAlterInfo::LinkDsn(const char* pszDSN)
    {
        //�������ö���
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        if(m_pConfig == NULL)
        {
            TADD_ERROR(-1,"LinkDsn() : Connect to DSN[%s] failed. Can't find Config-file.", pszDSN);
            return false;
        }
        m_pShmDSN= TMdbShmMgr::GetShmDSN(pszDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        
        if(m_pDsn == NULL)
            return false;
        else
            return true;

    }
    /******************************************************************************
    * ��������  :  AlterValue()
    * ��������  :  ���ݲ��������޸Ĳ���ֵ
    * ����      :  pszTable,pszName, pValue
    * ���      :  ��
    * ����ֵ    :  
    * ����      :  
    *******************************************************************************/
    int TMdbAlterInfo::AlterTable(char* pszTable,char* pszProperty,char* pszValue)
    {
        int iRet = 0;
        TADD_FUNC("TMdbAlterInfo::AlterTable() : start.");
        CHECK_OBJ(m_pDsn);
        //��������Ƿ�֧��
        SetSysAttr(false);//��ӱ�����
        std::map<string,string>::iterator itor = m_mapAttrName.find(pszProperty);
        if (itor != m_mapAttrName.end()) 
        { 
            SAFESTRCPY(m_sAttrName,sizeof(m_sAttrName),(itor->second).c_str());
        } 
        else
        { 
            TADD_ERROR(-1,"not support table attribute[%s].",pszProperty);
            printf("support table property:\n"
                                            "  bReadLock bWriteLock  \n"
                                            "  bIsCheckPriKey iLoadType m_sFilterSQL \n"
                                            "  m_sLoadSQL m_sFlushSQL bRollBack\n");
            
            return -1;
        }
        
        //���Ƿ����
        m_pMdbTable = m_pShmDSN->GetTableByName(pszTable);
        if(NULL == m_pMdbTable)
        {
            CHECK_RET(ERR_TAB_NO_TABLE,"no find Table= [%s].", m_pMdbTable->sTableName);
        }
        else
        {
            TADD_NORMAL("In-Memory Table = [%s].",m_pMdbTable->sTableName);
            SetTableValue(pszProperty,pszValue);
        }
        
        /*�޸������ļ���ʹ�ýű����Ľӿ���ʵ��*/
        Token tTable;//��
        Token tAttr;//����
        Token tValue;//����ֵ
        tTable.z    = pszTable;
        tTable.n    = strlen(pszTable);
        tAttr.z     = m_sAttrName;
        tAttr.n     = strlen(m_sAttrName);
        tValue.z    = pszValue;
        tValue.n    = strlen(pszValue);
        
        TMdbSqlParser tSqlParse;
        CHECK_RET(tSqlParse.NewDDLStruct(),"NewDDLStruct failed");
        CHECK_RET(tSqlParse.BuildModifyTableAttribute(1,&tTable,"modify"),"BuildModifyTableAttribute falied");
        CHECK_RET(tSqlParse.ModifyTableAttribute(&tAttr,&tValue),"ModifyTableAttribute failed");

        TMdbScript tMdbScript;
        CHECK_RET(tMdbScript.Init(&tSqlParse),"Init failed");
        CHECK_RET(tMdbScript.InitDsn(m_pDsn->sName,false),"InitDsn failed");
        CHECK_RET(tMdbScript.InitTableInfo(m_pDsn->sName,m_pMdbTable),"InitTableInfo failed");    
        CHECK_RET(tMdbScript.ModifyTableAttribute(false),"ModifyTableAttribute failed");
        TADD_FUNC("TMdbAlterInfo::AlterTable() : end.");
        return iRet;
    
    }
    /******************************************************************************
    * ��������  :  AlterColLen()
    * ��������  :  ���ݲ��������޸Ĳ���ֵ
    * ����      :  pszTable,pszName, pValue
    * ���      :  ��
    * ����ֵ    :  �ɹ�����true, ʧ�ܷ���false
    * ����      :  zhang.lin
    *******************************************************************************/
    bool TMdbAlterInfo::AlterColLen(char* pszTable,char* pszColumn,char* pszLen)
    {
        TADD_ERROR(-1,"not support to modify column \n");
        return false;
    }

    /******************************************************************************
    	* ��������  :  AlterValue()
    	* ��������  :  ���ݲ��������޸Ĳ���ֵ
    	* ����      :  pszTable,pszName, pValue
    	* ���      :  ��
    	* ����ֵ    :  
    	* ����      :  
    *******************************************************************************/
    int TMdbAlterInfo::AlterSys(char* pszProperty,char* pszValue)
    {
        int iRet = 0;
        TADD_FUNC("TMdbAlterInfo::AlterSys() : start.");
        //��������Ƿ�֧��
        CHECK_OBJ(m_pDsn);
        SetSysAttr(true);//���ϵͳ����
        std::map<string,string>::iterator itor = m_mapAttrName.find(pszProperty);
        if (itor != m_mapAttrName.end()) 
        { 
            SAFESTRCPY(m_sAttrName,sizeof(m_sAttrName),(itor->second).c_str());
        } 
        else
        { 
            TADD_ERROR(-1,"not support DSN attribute[%s].",pszProperty);
            printf("support sys property: iLogLevel iLogFileSize \n");
            return -1;
        }

        SetSysValue(pszProperty,pszValue);

        /*�޸������ļ���ʹ�ýű����Ľӿ���ʵ��*/
        Token tDsn;//dsn
        Token tAttr;//����
        Token tValue;//����ֵ
        tDsn.z    = m_pDsn->sName;
        tDsn.n    = strlen(m_pDsn->sName);
        tAttr.z     = m_sAttrName;
        tAttr.n     = strlen(m_sAttrName);
        tValue.z    = pszValue;
        tValue.n    = strlen(pszValue);
        
        TMdbSqlParser tSqlParse;
        CHECK_RET(tSqlParse.NewDDLStruct(),"NewDDLStruct failed");
        CHECK_RET(tSqlParse.BuildAlterDsn(1,&tDsn),"BuildAlterDsn falied");
        CHECK_RET(tSqlParse.AddDsnAttribute(&tAttr,&tValue),"AddDsnAttribute failed");
        
        TMdbScript tMdbScript;
        CHECK_RET(tMdbScript.Init(&tSqlParse),"Init failed");
        CHECK_RET(tMdbScript.InitDsn(m_pDsn->sName,false),"InitDsn failed");
        CHECK_RET(tMdbScript.ModifyDsn(false),"ModifyDsn failed");
        int iDsnPos = 0;
        CHECK_RET(tMdbScript.SaveSysCfgFile(iDsnPos),"ModifyDsn failed");
        TADD_FUNC("TMdbAlterInfo::AlterSys() : end.");
        return iRet;

    }


    /******************************************************************************
    	* ��������  :  SetTableValue()
    	* ��������  :  ��������������������ֵ
    	* ����      :  pszProperty,pszValue
    	* ���      :  ��
    	* ����ֵ    :  �� 
    	* ����      :  zhang.lin
    	*******************************************************************************/
    void TMdbAlterInfo::SetTableValue(char* pszProperty,char* pszValue)
    {   
        TADD_FUNC("TMdbAlterInfo::SetTableValue() : Start.");
        if(NULL == m_pMdbTable)
        {
            TADD_ERROR(-1,"m_pMdbTable is NULL.");
            return;
        }   
        if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "bReadLock") == 0)
        {
            TADD_NORMAL("before change: bReadLock = [%s] in table [%s].",BOOL_TO_STRING(m_pMdbTable->bReadLock),m_pMdbTable->sTableName);
            SET_PROPERTY_BOOL_VALUE(pszProperty,m_pMdbTable->bReadLock,pszValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "bWriteLock") == 0)
        {
            TADD_NORMAL("before change: bWriteLock = [%s] in table [%s].",BOOL_TO_STRING(m_pMdbTable->bWriteLock),m_pMdbTable->sTableName);
            SET_PROPERTY_BOOL_VALUE(pszProperty,m_pMdbTable->bWriteLock,pszValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "bIsCheckPriKey") == 0)
        {
            TADD_NORMAL("before change: bIsCheckPriKey = [%s] in table [%s].",BOOL_TO_STRING(m_pMdbTable->bIsCheckPriKey),m_pMdbTable->sTableName);
            SET_PROPERTY_BOOL_VALUE(pszProperty,m_pMdbTable->bIsCheckPriKey,pszValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "iLoadType") == 0)
        {
            TADD_NORMAL("before change: iLoadType = [%d] in table [%s].",m_pMdbTable->iLoadType,m_pMdbTable->sTableName);
            int iValue = atoi(pszValue);
            SET_PROPERTY_INT_VALUE(pszProperty,m_pMdbTable->iLoadType,iValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "m_sFilterSQL") == 0)
        {
            TADD_NORMAL("before change: m_sFilterSQL = [%s] in table [%s].",m_pMdbTable->m_sFilterSQL,m_pMdbTable->sTableName);
            SAFESTRCPY(m_pMdbTable->m_sFilterSQL, sizeof(m_pMdbTable->m_sFilterSQL), pszValue);
            TADD_NORMAL("after  change: m_sFilterSQL = [%s] in table [%s].",m_pMdbTable->m_sFilterSQL,m_pMdbTable->sTableName);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "m_sLoadSQL") == 0)
        {
            TADD_NORMAL("before change: m_sLoadSQL = [%s] in table [%s].",m_pMdbTable->m_sLoadSQL,m_pMdbTable->sTableName);
            SAFESTRCPY(m_pMdbTable->m_sLoadSQL, sizeof(m_pMdbTable->m_sLoadSQL), pszValue);
            TADD_NORMAL("after  change: m_sLoadSQL = [%s] in table [%s].",m_pMdbTable->m_sLoadSQL,m_pMdbTable->sTableName);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "m_sFlushSQL") == 0)
        {
            TADD_NORMAL("before change: m_sFlushSQL = [%s] in table [%s].",m_pMdbTable->m_sFlushSQL,m_pMdbTable->sTableName);
            SAFESTRCPY(m_pMdbTable->m_sFlushSQL, sizeof(m_pMdbTable->m_sFlushSQL), pszValue);
            TADD_NORMAL("after  change: m_sFlushSQL = [%s] in table [%s].",m_pMdbTable->m_sFlushSQL,m_pMdbTable->sTableName);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "bRollBack") == 0)
        {
            TADD_NORMAL("before change: bRollBack = [%s] in table [%s].",BOOL_TO_STRING(m_pMdbTable->bRollBack),m_pMdbTable->sTableName);
            SET_PROPERTY_BOOL_VALUE(pszProperty,m_pMdbTable->bRollBack,pszValue);
        }
        else
        {
            TADD_ERROR(-1,"Invalid table property[%s].",pszProperty);
        }
        TADD_FUNC("TMdbAlterInfo::SetTableValue() : End.");    
    }

    /******************************************************************************
    * ��������  :  SetSysValue()
    * ��������  :  ��������������������ֵ
    * ����      :  pszProperty,pszValue
    * ���      :  ��
    * ����ֵ    :  �� 
    * ����      :  zhang.lin
    *******************************************************************************/
    void TMdbAlterInfo::SetSysValue(char* pszProperty,char* pszValue)
    {   
        TADD_FUNC("TMdbAlterInfo::SetSysValue() : Start.");
        if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "iLogLevel") == 0)
        {
            TADD_NORMAL("before change: iLogLevel = [%d] in shared mem.",m_pDsn->iLogLevel);
            int iValue = atoi(pszValue);
            SET_PROPERTY_INT_VALUE(pszProperty,m_pDsn->iLogLevel,iValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszProperty, "iLogFileSize") == 0)
        {
            TMdbSyncArea & tSA = m_pDsn->m_arrSyncArea;
            TADD_NORMAL("before change: iLogFileSize = [%d] in shared mem.",tSA.m_iShmSize);
            int iValue = atoi(pszValue);
            SET_PROPERTY_INT_VALUE(pszProperty,tSA.m_iShmSize,iValue);
        }
        else
        {
            TADD_ERROR(-1,"Invalid sys property[%s].",pszProperty);
        }
        TADD_FUNC("TMdbAlterInfo::SetSysValue() : End.");
        
        
    }

    /******************************************************************************
    * ��������  :  SetProccessIsMonitor()
    * ��������  :  ����ָ�������Ƿ�QMDB��ؽ��̼��
    * ����      :  iPid ����ID,bMonitor �Ƿ���
    * ���      :  ��
    * ����ֵ    :  0 : �ɹ�����0 : ʧ��
    * ����      :  cao.peng
    *******************************************************************************/
    int  TMdbAlterInfo::SetProccessIsMonitor(const int iPid,const bool bMonitor)
    {
        int iRet = 0;
        //�������Ƿ����
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        //������״̬
        CHECK_RET(m_pShmDSN->LockDSN(),"Lock failed.");
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor; 
            if(tMdbProc.sName[0] == 0){continue;}
            if(tMdbProc.iPid == iPid)
            {
                tMdbProc.bIsMonitor = bMonitor;
                tMdbProc.Show();
                break;
            }
        }
        CHECK_RET(m_pShmDSN->UnLockDSN(),"UnLock failed.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  SetDumpPackage()
    * ��������  : ����mdbAgent ����״̬���Ƿ�ץȡ���ݰ�
    * ����      :  ����״̬
    * ���      :  
    * ����ֵ    :  
    * ����      :  
    *******************************************************************************/
    int  TMdbAlterInfo::SetDumpPackage(char cState)
    {
        int iRet = 0;
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        CHECK_RET(m_pShmDSN->LockDSN(),"Lock failed.");
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor; 
            if(tMdbProc.sName[0] == 0){continue;}
            if(strncmp(tMdbProc.sName, "mdbAgent", strlen("mdbAgent")) == 0)
            {
                tMdbProc.cState = cState;
                tMdbProc.Show();
                //break;
            }
        }
        CHECK_RET(m_pShmDSN->UnLockDSN(),"UnLock failed.");
        return iRet;
    }

void  TMdbAlterInfo::SetSysAttr(bool bSys)
{
    if(bSys)
    {
        m_mapAttrName["iLogLevel"]      = "log_level";
        m_mapAttrName["iLogFileSize"]   = "file_size";
    }
    else
    {
        m_mapAttrName["bReadLock"]      = "is_read_lock";
        m_mapAttrName["bWriteLock"]     = "is_write_lock";
        m_mapAttrName["bRollBack"]      = "is_rollback";
        m_mapAttrName["bIsCheckPriKey"] = "checkPriKey";
        m_mapAttrName["iLoadType"]      = "loadType";
        m_mapAttrName["m_sFilterSQL"]   = "filter_sql";
        m_mapAttrName["m_sLoadSQL"]     = "load_sql";
        m_mapAttrName["m_sFlushSQL"]    = "flush_sql";
        
    }
}
//}

