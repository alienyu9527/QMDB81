/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbAterInfo.h       
*@Description�� �����޸��ڴ����ݿ�ϵͳ������Ϣ
*@Author:       zhang.lin
*@Date��        2012��02��23��
*@History:
******************************************************************************************/
#ifndef __QUICK_MDB_ALTER_INFO_H__
#define __QUICK_MDB_ALTER_INFO_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{


#define SET_PROPERTY_INT_VALUE(PROPERTY,VARIABLE,VALUE)\
    if(VALUE <0 )\
    {\
        TADD_ERROR(-1,"invalid Integer Value[%s].",VALUE);\
        return -1;\
    }\
    else\
    {\
       VARIABLE =  VALUE;\
       TADD_NORMAL("after change: %s = [%d] in shared mem.",PROPERTY,VALUE);\
    }

#define SET_PROPERTY_BOOL_VALUE(PROPERTY,VARIABLE,VALUE)\
    if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "y") != 0 \
                && TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "n") != 0\
                && TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "true") != 0\
                && TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "false") != 0)\
    {\
        TADD_ERROR(-1,"invalid bool Value[%s]. valied value:[Y N TRUE FALSE]",VALUE);\
        return -1;\
    }\
    else\
    {\
        if(TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "y") == 0 \
            || TMdbNtcStrFunc::StrNoCaseCmp(VALUE, "true") == 0)\
        {\
            VARIABLE = true;\
        }\
        else\
        {\
            VARIABLE = false;\
            TADD_NORMAL("after change: %s = [%s] in shared mem.",PROPERTY,VARIABLE?"true":"false");\
        }\
    }

#define BOOL_TO_STRING(VALUE)  ((VALUE)?"true":"false")

    class TMdbAlterInfo
    {
    public:
        TMdbAlterInfo();
        ~TMdbAlterInfo();

    	/******************************************************************************
    	* ��������  :  LinkDsn()
    	* ��������  :  ����ĳ��DSN�����ǲ��ڹ�����ע���κ���Ϣ    
    	* ����      :  pszDSN, ��������������DSN 
    	* ���      :  ��
    	* ����ֵ    :  �ɹ�����true, ʧ�ܷ���false
    	* ����      :  zhang.lin
    	*******************************************************************************/
    	bool LinkDsn(const char* pszDSN);

    	/******************************************************************************
    	* ��������  :  AlterSys()
    	* ��������  :  ���ݲ��������޸�ϵͳ����ֵ
    	* ����      :  pszName, pValue
    	* ���      :  ��
    	* ����ֵ    :  �ɹ�����true, ʧ�ܷ���false
    	* ����      :  zhang.lin
    	*******************************************************************************/
    	int AlterSys(char* pszName,char* pValue);

    	/******************************************************************************
    	* ��������  :  AlterTable()
    	* ��������  :  ���ݲ��������޸Ĳ���ֵ
    	* ����      :  pszTable,pszProperty, pszValue
    	* ���      :  ��
    	* ����ֵ    :  �ɹ�����true, ʧ�ܷ���false
    	* ����      :  zhang.lin
    	*******************************************************************************/
    	int AlterTable(char* pszTable,char* pszProperty,char* pszValue);

    	/******************************************************************************
    	* ��������  :  AlterColLen()
    	* ��������  :  ���ݱ���&���������ó���
    	* ����      :  pszTable,pszColumn, pszLen
    	* ���      :  ��
    	* ����ֵ    :  �ɹ�����true, ʧ�ܷ���false
    	* ����      :  zhang.lin
    	*******************************************************************************/
    	bool AlterColLen(char* pszTable,char* pszColumn,char* pszLen);
    	int  SetProccessIsMonitor(const int iPid,const bool bMonitor);//���ý����Ƿ񱸼��
    	int  SetDumpPackage(char cState);//����CS ģʽ�Ƿ�ץȡ���ݰ�
        
    private:
    	/******************************************************************************
    	* ��������  :  SetTableValue()
    	* ��������  :  ��������������������ֵ
    	* ����      :  pszProperty,pszValue
    	* ���      :  ��
    	* ����ֵ    :  �� 
    	* ����      :  zhang.lin
    	*******************************************************************************/
    	int SetTableValue(char* pszProperty,char* pszValue);


    	/******************************************************************************
    	* ��������  :  SetSysValue()
    	* ��������  :  ��������������������ֵ
    	* ����      :  pszProperty,pszValue
    	* ���      :  ��
    	* ����ֵ    :  �� 
    	* ����      :  zhang.lin
    	*******************************************************************************/
    	int SetSysValue(char* pszProperty,char* pszValue);


    	/******************************************************************************
    	* ��������  :  SetSysAttr()
    	* ��������  : ������ԣ�
    	* ����      :  bSys,true:ϵͳ����,false:������
    	* ���      :  ��
    	* ����ֵ    :  �� 
    	* ����      :  wang.liebao
    	*******************************************************************************/
        void SetSysAttr(bool bSys);//

    private:
        TMdbConfig *m_pConfig;
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn;     
        TMdbTable  *m_pMdbTable;
        TMdbTableSpace *m_pMdbTableSpace;
    	TMdbColumn *m_pColumn;
        char m_sAttrName[MAX_NAME_LEN];//DDL �﷨��Ӧ���������֣��� log_level
        std::map<string,string> m_mapAttrName;//�ڴ���������DDL�������ԵĶ�Ӧ��ϵ
    };

//}


#endif //__QUICK_MDB_ALTER_INFO_H__



