/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@                     All rights reserved.
*@Name��	    mdbClientQuery.cpp
*@Description�� ����CS���ʷ�ʽ�Ľӿ�
*@Author:	jin.shaohua
*@Date��	    2012.06
*@History:
******************************************************************************************/
#include "Interface/mdbQuery.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbBase.h"
#include "Helper/SqlParserStruct.h"
#include "Helper/Tokenize.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "Helper/mdbSocket.h"
#include "Helper/mdbCspAvpMgr.h"
#include "Interface/mdbClientQuery.h"
#include "Helper/mdbMultiProtector.h"
#include "Common/mdbIniFiles.h"
#include "Common/mdbCommons.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;


#ifdef WIN32
#include <iostream>
#include <windows.h>
#include <sys/types.h>
#pragma warning(disable:4101)
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <limits.h>
//#include <stropts.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <pthread.h>
#endif



//namespace QuickMDB{

    #define TIME_OUT_TIME 20

    #ifdef OS_SUN
        #define CHECK_RET_THROW(_ret,_errCode,_sql,...)  if((iRet = _ret)!=0){\
            TADD_ERROR(iRet,__VA_ARGS__);\
            throw TMdbException(_errCode,_sql,__VA_ARGS__);\
            }

        #define CHECK_RET_THROW_NOSQL(_ret,_errCode,...)  if((iRet = _ret)!=0){\
            TADD_ERROR(iRet,__VA_ARGS__);\
            throw TMdbException(_errCode,"",__VA_ARGS__);\
            }

        #define ERROR_TO_THROW_NOSQL(_errCode,...) \
            TADD_ERROR(_errCode,__VA_ARGS__);\
            throw TMdbException(_errCode,"",__VA_ARGS__);\
     
        #define ERROR_TO_THROW(_errCode,_sql,...) \
            TADD_ERROR(_errCode,__VA_ARGS__);\
            throw TMdbException(_errCode,_sql,__VA_ARGS__);
    #else

        #define CHECK_RET_THROW(_ret,_errCode,_sql,FMT,...)  if((iRet = _ret)!=0){\
            TADD_ERROR(iRet,FMT,##__VA_ARGS__);\
            throw TMdbException(_errCode,_sql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\
            }

        #define CHECK_RET_THROW_NOSQL(_ret,_errCode,FMT,...)  if((iRet = _ret)!=0){\
            TADD_ERROR(iRet,FMT,##__VA_ARGS__);\
            throw TMdbException(_errCode,"","File=[%s], Line=[%d],"FMT,__FILE__,__LINE__,##__VA_ARGS__);\
            }

        #define ERROR_TO_THROW_NOSQL(_errCode,FMT,...) \
            TADD_ERROR(_errCode,FMT,##__VA_ARGS__);\
            throw TMdbException(_errCode,"","File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\

        #define ERROR_TO_THROW(_errCode,_sql,FMT,...) \
            TADD_ERROR(_errCode,FMT,##__VA_ARGS__);\
            throw TMdbException(_errCode,_sql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);
    #endif


    /******************************************************************************
    * ��������	:  TMdbClientField
    * ��������	:  CSģʽ��Field,���캯��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbClientField::TMdbClientField()
    {
        m_bIsNULL = false;
        m_iValue  = 0;
        m_sValue  = NULL;
        m_sName   = NULL;
        m_iDataType = 0;
        m_sNullValue[0] = 0;
        m_bUseOcp = true;
    }
    /******************************************************************************
    * ��������	:  TMdbClientField
    * ��������	:  CSģʽ��Field,����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbClientField::~TMdbClientField()
    {

    }
    /******************************************************************************
    * ��������	:  isNULL
    * ��������	:  �ж�Field�Ƿ�ΪNULL
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  true - NULL,false -��NULL
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbClientField::isNULL()
    {
        return m_bIsNULL;
    }
    /******************************************************************************
    * ��������	:  AsString
    * ��������	:
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    char* TMdbClientField::AsString() throw (TMdbException)
    {
        if(m_bIsNULL)
        {
            //ERROR_TO_THROW_NOSQL(ERR_SQL_DATA_TYPE_INVALID,"Column=[%s] is NULL.",m_sName);
            return m_sNullValue;
        }
        else
        {
            return m_sValue;
        }
    }

    /******************************************************************************
    * ��������	:  AsFloat
    * ��������	:
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    double TMdbClientField::AsFloat() throw (TMdbException)
    {
        if(m_bIsNULL)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_DATA_TYPE_INVALID,"Column=[%s] is NULL.",m_sName);
        }
        return atof(m_sValue);
    }

    /******************************************************************************
    * ��������	:  AsInteger
    * ��������	:
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    long long  TMdbClientField::AsInteger() throw (TMdbException)
    {
        if(m_bIsNULL)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_DATA_TYPE_INVALID,"Column=[%s] is NULL.",m_sName);
        }
        if(m_bUseOcp)
        {
            m_iValue = TMdbNtcStrFunc::StrToInt(m_sValue);
        }
        return m_iValue;
    }

    /******************************************************************************
    * ��������	:  DataType
    * ��������	:  ��������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbClientField::DataType(void)
    {
        return m_iDataType;
    }

    /******************************************************************************
    * ��������	:  AsName
    * ��������	:  field������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    const char * TMdbClientField::GetName()
    {
        return (const char*)m_sName;
    }

    /******************************************************************************
    * ��������	:  AsDateTime
    * ��������	:  �������ڵĸ�������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void  TMdbClientField::AsDateTime(int &iYear,int &iMonth,int &iDay,int &iHour,int &iMinute,int &iSecond) throw (TMdbException)
    {
        if(m_bIsNULL)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_DATA_TYPE_INVALID,"Column=[%p] is NULL.",m_sName);
        }
        iYear  = (m_sValue[0]-'0')*1000 + (m_sValue[1]-'0')*100 + (m_sValue[2]-'0')*10 + (m_sValue[3]-'0');
        iMonth = (m_sValue[4]-'0')*10 + (m_sValue[5]-'0');
        iDay   = (m_sValue[6]-'0')*10 + (m_sValue[7]-'0');
        iHour  = (m_sValue[8]-'0')*10 + (m_sValue[9]-'0');
        iMinute = (m_sValue[10]-'0')*10 + (m_sValue[11]-'0');
        iSecond = (m_sValue[12]-'0')*10 + (m_sValue[13]-'0');
    }
    /******************************************************************************
    * ��������	:  AsDateTime
    * ��������	:  ��������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  char * - ����
    * ����		:  jin.shaohua
    *******************************************************************************/
    char* TMdbClientField::AsDateTimeString() throw (TMdbException)
    {
        return m_sValue;
    }
    /******************************************************************************
    * ��������	:  AsBlobBuffer
    * ��������	:  ��ȡblobֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientField::AsBlobBuffer(unsigned char *buffer, int &iBufferLen) throw (TMdbException)
    {
        //Base64����
        std::string encoded = m_sValue;
        std::string decoded = Base::base64_decode(encoded);
        iBufferLen = decoded.length();
        memcpy(buffer,decoded.c_str(),decoded.length());
    }

    /******************************************************************************
    * ��������	:  ClearDataBuf
    * ��������	:  �������ݻ���
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientField::ClearDataBuf()
    {
        m_iValue = 0;                //�洢������
        m_sName = NULL;
        m_sValue = NULL;
        m_iDataType = -1;             //�������ͣ�1-Int, 2,3-Char, 4-Date
        m_bIsNULL = false;                //�Ƿ�Ϊ��
        m_sNullValue[0] = 0;
    }
    
    void TMdbClientField::ClearDataBufPlus()
    {
        m_iValue = 0;                //�洢������
        //m_sName = NULL;
        //m_sValue = NULL;
        //m_iDataType = -1;             //�������ͣ�1-Int, 2,3-Char, 4-Date
        m_bIsNULL = false;                //�Ƿ�Ϊ��
        m_sNullValue[0] = 0;
    }
    /******************************************************************************
    * ��������	:  TMdbClientQuery
    * ��������	:  CS ģʽ��ѯ�ӿ�
    * ����		:  pTMdbClientDatabase - CS��db.
    * ����		:  iSQLLabel - SQL���
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbClientQuery::TMdbClientQuery(TMdbClientDatabase *pTMdbClientDatabase,int iSQLLabel)
    {
        m_iSQLLabel = iSQLLabel;
        m_pMdb      = pTMdbClientDatabase;
        if(m_pMdb == NULL)
        {
            ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"pTMdbClientDatabase is NULL.");
        }
		m_iUseOcp = m_pMdb->GetUseOcpFlag();
		m_pCspParserMgr = NULL;
		m_pCspSetSQL = NULL;
		m_pCspSetParam = NULL;
		m_pCspSetParamArray = NULL;
		m_pCspSetSQLAns = NULL;
		m_pCspNext = NULL;
		m_pCspError = NULL;
		m_pCspSeqSend = NULL;
		m_pCspSeqRecv= NULL;
        if(MDB_CS_USE_OCP == m_iUseOcp)
        {
            //��ʼ��������
            m_pCspParserMgr = new (std::nothrow) TMdbCspParserMgr();
            if(NULL == m_pCspParserMgr)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pCspParserMgr is NULL.");
            }
            m_pCspSetSQL    = m_pCspParserMgr->GetParserByType(CSP_APP_SEND_SQL,true);
            m_pCspSetParam  = m_pCspParserMgr->GetParserByType(CSP_APP_SEND_PARAM,true);
            m_pCspSetParamArray = m_pCspParserMgr->GetParserByType(CSP_APP_SEND_PARAM_ARRAY,true);
            m_pCspSetSQLAns = m_pCspParserMgr->GetParserByType(CSP_APP_SEND_SQL,false);
            m_pCspNext		= m_pCspParserMgr->GetParserByType(CSP_APP_NEXT,true);
            m_pCspError		= m_pCspParserMgr->GetParserByType(CSP_APP_ERROR,true);
            m_pCspSeqSend	= m_pCspParserMgr->GetParserByType(CSP_APP_GET_SEQUENCE,true);
            m_pCspSeqRecv	= m_pCspParserMgr->GetParserByType(CSP_APP_GET_SEQUENCE,false);
        }
        m_pCurRowGroup  = NULL;
        m_iIsNeedNextOper = 1;
        m_bIsDynamic    = false;
        m_fLastRecordFlag = false;
        m_bIsParamArray = false;
        m_iFiledCounts = 0;
        m_bFirstNext = true;
        m_pParamArray = NULL;
        m_pHead = new (std::nothrow) TMdbAvpHead();
        if(NULL == m_pHead)
        {
            ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pHead is NULL.");
        }
        m_bCanOpenAgain = false;
		m_sSQL = new char[MAX_SQL_LEN];
		if(m_sSQL == NULL)
		{
			ERROR_TO_THROW(ERR_OS_NO_MEMROY,m_sSQL,"Mem Not Enough.");
		}
		memset(m_sSQL, 0, MAX_SQL_LEN);
		m_iSQLBuffLen = MAX_SQL_LEN;
    }

    /******************************************************************************
    * ��������	:  ~TMdbClientQuery
    * ��������	:  ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbClientQuery::~TMdbClientQuery()
    {
        CloseSQL();
        size_t i = 0;
        for(i = 0; i<m_vField.size(); ++i)
        {
            if(MDB_CS_USE_BIN == m_iUseOcp)
            {
                SAFE_DELETE(m_vField[i]->m_sName);
                SAFE_DELETE(m_vField[i]->m_sValue);
            }
            SAFE_DELETE(m_vField[i]);
        }
        m_vField.clear();
        SAFE_DELETE(m_pCspParserMgr);
        SAFE_DELETE(m_pHead);
		SAFE_DELETE(m_sSQL);
    }
    /******************************************************************************
    * ��������	:  Close
    * ��������	:  �ر�SQL
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientQuery::Close()
    {
        CloseSQL();
    }
    /******************************************************************************
    * ��������	:  Close
    * ��������	:  �ر�SQL
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientQuery::CloseSQL()
    {
        TADD_FUNC("Start.");
        memset(m_sSQL, 0, m_iSQLBuffLen);
        if(m_pCspSetSQL)
        {
            m_pCspSetSQL->Clear();
        }
        if(m_pCspSetParam)
        {
            m_pCspSetParam->Clear();
        }
        if(m_pCspSetParamArray)
        {
            m_pCspSetParamArray->Clear();
        }
        if(m_pCspSetSQLAns)
        {
            m_pCspSetSQLAns->Clear();
        }
        if(m_pCspNext)
        {
            m_pCspNext->Clear();
        }
        if(m_pCspError)
        {
            m_pCspError->Clear();
        }
        if(m_pCspSeqSend)
        {
            m_pCspSeqSend->Clear();
        }
        if(m_pCspSeqRecv)
        {
            m_pCspSeqRecv->Clear();
        }
        m_iIsNeedNextOper = 1;
        m_bIsDynamic      = false;
        m_fLastRecordFlag = false;
        m_iFiledCounts = 0;
        m_tParam.Clear();//����󶨱���ӳ��
        m_bIsParamArray = false;
        m_bCanOpenAgain = false;
        m_bFirstNext = true;
        SAFE_DELETE_ARRAY(m_pParamArray);
        TADD_FUNC("Finish.");

    }
    /******************************************************************************
    * ��������	: SetSQL
    * ��������	:  ����Ҫִ�е�SQL
    * ����		:  sSqlStatement - sql ���
    * ����		:  iPreFetchRows - ��ʱ����
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientQuery::SetSQL(const char *sSqlStatement ,int iPreFetchRows) throw (TMdbException)
    {
        SetSQL(sSqlStatement,0,iPreFetchRows);
    }

    /******************************************************************************
    * ��������	: SetSQL
    * ��������	:  ����Ҫִ�е�SQL
    * ����		:  sSqlStatement - sql ��� - bRBFlag �Ƿ�����ͬ���ı�ʶ
    * ����		:  iPreFetchRows - ��ʱ����
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientQuery::SetSQL(const char *sSqlStatement,int iSqlFlag ,int iPreFetchRows) throw (TMdbException)
    {
        m_iUseOcp = m_pMdb->GetUseOcpFlag();
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetSQLBin(sSqlStatement,iSqlFlag,iPreFetchRows);
            return;
        }
        TADD_FUNC("SQL=[%s],Flag=[%d],fetchRows=[%d].",sSqlStatement,iSqlFlag,iPreFetchRows);
        int iRet = 0;
        if(m_sSQL == NULL)
		{
			m_sSQL = new char[MAX_SQL_LEN];
			memset(m_sSQL, 0, MAX_SQL_LEN);
			m_iSQLBuffLen = MAX_SQL_LEN;
		}
	    CloseSQL();
		int sqlLen = strlen(sSqlStatement);
	    if(sqlLen >= m_iSQLBuffLen)
		{
			SAFE_DELETE(m_sSQL);
			m_sSQL = new char[sqlLen + 100];
			if(m_sSQL == NULL)
			{
				ERROR_TO_THROW(ERR_OS_NO_MEMROY,m_sSQL,"Mem Not Enough.");
			}
			memset(m_sSQL, 0, sqlLen + 100);
			m_iSQLBuffLen = sqlLen + 100;
		}
	    
	    SAFESTRCPY(m_sSQL,m_iSQLBuffLen,sSqlStatement);
        m_bIsDynamic = IsDynamicSQL(sSqlStatement);//�Ƿ��Ƕ�̬������߿���������
        //ȥ��ĩβ��;
        int ilen = strlen(m_sSQL);
        bool isFound = false;
        while(m_sSQL[ilen] == ';')
        {
            isFound = true;
            ilen--;
        }
        if(isFound == true)
            m_sSQL[ilen+1] = '\0';
        else
            m_sSQL[ilen] = '\0';
        TADD_DETAIL("SQL=[%s],m_bIsDynamic=[%d].",m_sSQL,m_bIsDynamic);
        try
        {
            m_iSQLType = iSqlFlag;
            m_pCspSetSQL->Clear();
            m_pCspSetSQL->SetItemValue(m_pCspSetSQL->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)m_iSQLLabel);
            m_pCspSetSQL->SetItemValue(m_pCspSetSQL->m_pRootAvpItem,AVP_SQL_STATEMENT,m_sSQL);
            m_pCspSetSQL->SetItemValue(m_pCspSetSQL->m_pRootAvpItem,AVP_SQL_TYPE,0);
            m_pCspSetSQL->SetItemValue(m_pCspSetSQL->m_pRootAvpItem,AVP_SQL_FLAG,iSqlFlag);

            m_pCspSetSQL->Serialize(m_sSendPackage,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            //����sql���
            iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_pCspSetSQL->m_tHead.iLen);//stocket����д����
            if(iRet < 0)
            {
                ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"send(%s:%d) failed, SocketID is invalid!",m_pMdb->m_sIP, m_pMdb->m_iPort);
            }
            TADD_DETAIL("Send SetSQL OK.");
            m_pMdb->RecvPackage(CSP_APP_SEND_SQL,m_sRecvPackage,*m_pHead,m_pCspError);
            if(m_pHead->iCmdCode == CSP_APP_SEND_SQL)
            {
                // TMdbStrFunc::PrintMsg(m_sRecvPackage, m_pHead->iLen);//��ӡ
                m_pCspSetSQLAns->DeSerialize(m_sRecvPackage, m_pHead->iLen);
                int sqllabel = m_pCspSetSQLAns->GetUINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_SQL_LABEL);
                if(sqllabel != m_iSQLLabel)
                {
                    ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"SQL label not equal,sqllabel[%d] != m_iSQLLabel[%d]",sqllabel,m_iSQLLabel);
                }
                m_pMdb->CheckAnsCode(m_pCspSetSQLAns);
                m_iIsNeedNextOper = m_pCspSetSQLAns->GetINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT); //m_pSQLOperAnswerMgrRecv->GetSelectHaveNext();
                if(m_bIsDynamic)
                {
                    //���Ƕ�̬sqlһ��Ҫ��Զ˻�ȡ
                    m_iIsNeedNextOper = 1;
                    m_bCanOpenAgain = true;
                }
                else
                {
                    m_bCanOpenAgain = false;//��̬SQL��setSQL��ʱ��ִ�й�һ����
                }
                m_iRowsAff		  = m_pCspSetSQLAns->GetINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW);//��Ӱ���rows
                m_pCurRowGroup = m_pCspSetSQLAns->FindExistAvpItem(m_pCspSetSQLAns->m_pRootAvpItem,AVP_ROW_GROUP);//�ҵ���һ��row group
                TADD_DETAIL("m_iIsNeedNextOper=[%d],m_iRowsAff=[%d],m_pCurRowGroup=[%p].",m_iIsNeedNextOper,m_iRowsAff,																m_pCurRowGroup);
            }
        }

        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"TMdbCSPException [%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.");
    }

    /******************************************************************************
    * ��������	:  IsDynamicSQL
    * ��������	:  �ж��Ƿ��Ƕ�̬SQL
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbClientQuery::IsDynamicSQL(const char * sSQL)
    {
        bool bRet  = false;
        int iRet = 0;
        if(NULL == sSQL)
        {
            return false;
        }
        int tokenType;
        Token sLastToken;
        memset(&sLastToken,0x00,sizeof(Token));
        int i = 0;
        m_tParam.Clear();
        char sParamName[64] = {0};
        while( sSQL[i] != 0)
        {
            sLastToken.z = &sSQL[i];
            sLastToken.n = GetToken((unsigned char*)&sSQL[i],&tokenType);
            i += sLastToken.n;
            if(TK_VARIABLE == tokenType)
            {
                bRet = true;
                memset(sParamName,0,sizeof(sParamName));
                strncpy(sParamName,sLastToken.z+1,sLastToken.n-1);
                CHECK_RET_THROW(m_tParam.AddParam(sParamName),ERR_SQL_INVALID,sSQL,"bind param[%s] error",sParamName);
            }
        }
        m_tParam.NewParamPool();
        m_pParamArray = new  TMdbParamArray[m_tParam.GetCount()];
        return bRet;
    }

    /******************************************************************************
    * ��������	:  SendParam
    * ��������	:  ���Ͳ�������
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    int TMdbClientQuery::SendParam() throw (TMdbException)
    {
        int iRet = 0;
        if(false == m_bIsParamArray)
        {
            //����
            m_pCspSetParam->SetItemValue(m_pCspSetParam->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)m_iSQLLabel);
            m_pCspSetParam->Serialize(m_sSendPackage,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_pCspSetParam->m_tHead.iLen);//stocket����д����
            m_pCspSetParam->Clear();
        }
        else
        {
            //����
            int iBatchCount = m_pParamArray[0].m_iArraySize;
            int iParamCount  = m_tParam.GetCount();
            int i = 0;
            char sTempNum[64] = {0};
            m_pCspSetParamArray->Clear();
            try
            {
                TMdbAvpItem * pItem = m_pCspSetParamArray->m_pRootAvpItem;
                for(i = 0; i < iBatchCount; ++i)
                {
                    int iBatchGroupLen = m_pCspSetParamArray->GetTotalLen();
                    pItem = m_pCspSetParamArray->GetFreeAvpItem(pItem,AVP_BATCH_GROUP);
                    if(NULL == pItem)
                    {
                        ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"GetFreeAvpItem faild");
                    }
                    int iParamPos;
                   TMdbAvpItem * pParamItem = pItem->pChildItem;
                    for(iParamPos = 0; iParamPos < iParamCount; ++iParamPos)
                    {
                        //int iParamGroupLen = m_pCspSetParamArray->GetTotalLen();
                        pParamItem = m_pCspSetParamArray->GetFreeAvpItem(pParamItem,AVP_PARAM_STR_GROUP);
                        //m_pCspSetParamArray->SetItemValue(pParamItem->pChildItem,AVP_PARAM_NAME,m_tParam.GetParamNameByIndex(iParamPos));
                        if(m_pParamArray[iParamPos].m_tNullArrWrap.IsNull(i))
                        {
                            //NULLֵ
                            m_pCspSetParamArray->SetItemValueNULL(pParamItem,AVP_PARAM_STR_GROUP);
                        }
                        else
                        {
                            switch(m_pParamArray[iParamPos].m_iParamType)
                            {
                            case MEM_Int:
                                sprintf(sTempNum,"%lld",m_pParamArray[iParamPos].m_pllValue[i]);
                                //m_pCspSetParamArray->SetItemValue(pParamItem->pChildItem,AVP_PARAM_VALUE,sTempNum);
                                m_pCspSetParamArray->SetItemValueForParamGroup(pParamItem,AVP_PARAM_STR_GROUP,sTempNum);
                                break;
                            case MEM_Str:
                                m_pCspSetParamArray->SetItemValueForParamGroup(pParamItem,AVP_PARAM_STR_GROUP,m_pParamArray[iParamPos].m_psValue[i]);
                                break;
                            default:
                                ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND,m_sSQL,"Param[%d]Type[%d] error.",i,
                                               m_pParamArray[i].m_iParamType);
                                break;
                            }
                        }
                        //m_pCspSetParamArray->FinishFillGroup(pParamItem,m_pCspSetParamArray->GetTotalLen()-iParamGroupLen);
                        //pParamItem->FinishFill();
                    }
                    m_pCspSetParamArray->FinishFillGroup(pItem,m_pCspSetParamArray->GetTotalLen()-iBatchGroupLen);
                    //pItem->FinishFill();
                }
                m_pCspSetParamArray->SetItemValue(m_pCspSetParamArray->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)m_iSQLLabel);
                m_pCspSetParamArray->Serialize(m_sSendPackage,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
                iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_pCspSetParamArray->m_tHead.iLen);//stocket����д����
                m_pCspSetParamArray->Clear();
                
            }
            catch(TMdbCSPException& e)
            {
                ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
            }
            catch(...)
            {
                ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
            }
        }
        return iRet;
    }

    int TMdbClientQuery::ParamCount()
    {
        return m_tParam.GetCount();
    }//��ȡ�󶨲�������
    const char * TMdbClientQuery::GetParamNameByIndex(int iIndex)
    {
        return m_tParam.GetParamNameByIndex(iIndex);
    }//����idx��ȡ�󶨲�����
    int TMdbClientQuery::GetParamIndexByName(const char * sName)
    {
        return m_tParam.GetParamIndexByName(sName);
    }//����name����ȡ�󶨲���index


    /******************************************************************************
    * ��������	:  Open
    * ��������	:  ��SQL SELECT��䷵�ؽ����,iPreFetchRows��ʵ������
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    void TMdbClientQuery::Open(int prefetchRows) throw (TMdbException)
    {
        m_tParam.InitParamPool();
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            OpenBin(prefetchRows);
            return;
        }
    
        TADD_FUNC("Start.m_bIsDynamic=[%d],m_bCanOpenAgain = [%d].",m_bIsDynamic,m_bCanOpenAgain);
        int iRet = 0;
        try
        {
            //��̬sql �����ٴ�OPEN
            if(m_bCanOpenAgain)
            {
                iRet = SendParam();
                if(iRet < 0)
                {
                    ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"Open(%s:%d):send()failed, SocketID is invalid!",m_pMdb->m_sIP, m_pMdb->m_iPort);
                }
                TADD_DETAIL("Send SetParam OK.");
                m_pMdb->RecvPackage(CSP_APP_SEND_SQL,m_sRecvPackage,*m_pHead,m_pCspError);
                if(m_pHead->iCmdCode == CSP_APP_SEND_SQL)
                {
                    m_pCspSetSQLAns->DeSerialize(m_sRecvPackage, m_pHead->iLen);
                    int sqllabel = m_pCspSetSQLAns->GetUINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_SQL_LABEL);
                    if(sqllabel != m_iSQLLabel)
                    {
                        ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"SQL label not equal,sqllabel[%d] != m_iSQLLabel[%d]",sqllabel,m_iSQLLabel);
                    }
                    m_pMdb->CheckAnsCode(m_pCspSetSQLAns);
                    m_iIsNeedNextOper = m_pCspSetSQLAns->GetINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT); //m_pSQLOperAnswerMgrRecv->GetSelectHaveNext();
                    m_iRowsAff        = m_pCspSetSQLAns->GetINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW);//��Ӱ���rows
                    m_pCurRowGroup = m_pCspSetSQLAns->FindExistAvpItem(m_pCspSetSQLAns->m_pRootAvpItem,AVP_ROW_GROUP);//�ҵ���һ��row group
                    TADD_DETAIL("m_iIsNeedNextOper=[%d],m_iRowsAff=[%d],m_pCurRowGroup=[%p].",m_iIsNeedNextOper,m_iRowsAff,
                                m_pCurRowGroup);
                }
            }
            m_bCanOpenAgain = true;//�����ٴα�open
        }
        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.");
    }

    /******************************************************************************
    * ��������	:  Next
    * ��������	:  �ƶ�����һ����¼
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:
    *******************************************************************************/
    bool TMdbClientQuery::Next()throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            return NextBin();
        }
    
        TADD_FUNC("Start.m_iIsNeedNextOper=[%d].",m_iIsNeedNextOper);
        int  iRet = 0;
        bool bRet = false;
        try
        {

            if(FillFieldValue() != 0)
            {
                //û��next ֵ��������Ҫ��Զ������ȡ
                if(m_iIsNeedNextOper == 0)
                {
                    //�Զ�Ҳ��ʾû��û��������
                    bRet =  false;
                    m_fLastRecordFlag = true;
                }
                else
                {
                    m_pCspNext->Clear();
                    m_pCspNext->SetItemValue(m_pCspNext->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)m_iSQLLabel);
                    m_pCspNext->Serialize(m_sSendPackage,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
                    iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_pCspNext->m_tHead.iLen);//stocket����д����
                    if(iRet < 0)
                    {
                        ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"send() failed, SocketID is invalid!");
                    }
                    TADD_DETAIL("Send Next OK.");
                    m_pMdb->RecvPackage(CSP_APP_SEND_SQL,m_sRecvPackage,*m_pHead,m_pCspError);
                    if(m_pHead->iCmdCode == CSP_APP_SEND_SQL)
                    {
                        m_pCspSetSQLAns->DeSerialize(m_sRecvPackage, m_pHead->iLen);
                        int sqllabel = m_pCspSetSQLAns->GetUINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_SQL_LABEL);
                        if(sqllabel != m_iSQLLabel)
                        {
                            ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"SQL label not equal,sqllabel[%d] != m_iSQLLabel[%d]",sqllabel,m_iSQLLabel);
                        }
                        m_pMdb->CheckAnsCode(m_pCspSetSQLAns);
                        m_iRowsAff        += m_pCspSetSQLAns->GetINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW);//��Ӱ���rows
                        m_iIsNeedNextOper = m_pCspSetSQLAns->GetINT32Value(m_pCspSetSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT); //m_pSQLOperAnswerMgrRecv->GetSelectHaveNext();
                        m_pCurRowGroup = m_pCspSetSQLAns->FindExistAvpItem(m_pCspSetSQLAns->m_pRootAvpItem,AVP_ROW_GROUP);//�ҵ���һ��row group
                        TADD_DETAIL("m_iIsNeedNextOper=[%d],m_pCurRowGroup=[%p].",m_iIsNeedNextOper,m_pCurRowGroup);
                        return Next();
                    }
                }
            }
            else
            {
                bRet =  true;
            }
        }
        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.ret=[%d].",bRet);
        return bRet;
    }

    /******************************************************************************
    * ��������	:  Execute
    * ��������	:  ִ��SQL
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbClientQuery::Execute(int iExecuteRows) throw (TMdbException)
    {
        TADD_FUNC("Start.");
        Open(iExecuteRows);//ֱ�ӵ���open
        TADD_FUNC("Finish.");
        return true;
    }

    /******************************************************************************
    * ��������	:  GetSequenceByName
    * ��������	:  ����name ��ȡ����ֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    long long TMdbClientQuery::GetSequenceByName(const char * sName)throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            return GetSequenceByNameBin(sName);
        }
    
        TADD_FUNC("Start.SeqName=[%s].",sName);
        int iRet = 0;
        long long llSeqValue = 0;
        try
        {
            m_pCspSeqSend->Clear();
            m_pCspSeqSend->SetItemValue(m_pCspSeqSend->m_pRootAvpItem,AVP_SEQUENCE_NAME,sName);
            m_pCspSeqSend->Serialize(m_sSendPackage,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_pCspSeqSend->m_tHead.iLen);//stocket����д����
            if(iRet < 0)
            {
                ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"Open(%s:%d):send()failed, SocketID is invalid!",m_pMdb->m_sIP, m_pMdb->m_iPort);
            }
            TADD_DETAIL("Send GetSeq OK.");
            m_pMdb->RecvPackage(CSP_APP_GET_SEQUENCE,m_sRecvPackage,*m_pHead,m_pCspError);
            if(m_pHead->iCmdCode == CSP_APP_GET_SEQUENCE)
            {
                m_pCspSeqRecv->DeSerialize(m_sRecvPackage, m_pHead->iLen);
                m_pMdb->CheckAnsCode(m_pCspSeqRecv);
                llSeqValue = m_pCspSeqRecv->GetINT64Value(m_pCspSeqRecv->m_pRootAvpItem,AVP_SEQUENCE_VALUE);
            }
        }
        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.SeqValue=[%ld].",llSeqValue);
        return llSeqValue;
    }

    //������
    bool TMdbClientQuery::TransBegin()
    {
        return true;
    }

    bool TMdbClientQuery::Eof()
    {
        //return m_iIsNeedNextOper == 1?false:true;
        TADD_DETAIL("%s.", m_fLastRecordFlag?"TRUE":"FALSE");
        return m_fLastRecordFlag;
    }

    //�����ύ
    bool TMdbClientQuery::Commit()
    {
        m_pMdb->Commit();
        return true;
    }


    //����ع�
    bool TMdbClientQuery::Rollback()
    {

        m_pMdb->Rollback();

        return true;
    }


    //DELETE/UPDATE/INSERT����޸ĵļ�¼��Ŀ,SELECT���ĿǰNext֮��ļ�¼��
    int TMdbClientQuery::RowsAffected()
    {
        return m_iRowsAff;
    }

    //��ȡ�и���
    int TMdbClientQuery::FieldCount()
    {
        return m_iFiledCounts;
    }


    /******************************************************************************
    * ��������	:  FillFieldValue
    * ��������	:  ����ʣ���rowgroup �����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbClientQuery::FillFieldValue()throw (TMdbException)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            if(NULL != m_pCurRowGroup)
            {
                std::vector<TMdbAvpItem * > vColumnValue ;
                m_pCspSetSQLAns->GetExistGroupItem(m_pCurRowGroup->pChildItem,AVP_COLUMN_GROUP, vColumnValue);
                std::vector<TMdbAvpItem * >::iterator itorNew =  vColumnValue.begin();
                m_iFiledCounts = vColumnValue.size();
                int iOldFieldCounts = m_vField.size();
                TADD_DETAIL("iOldFieldCounts=[%d],m_iFiledCounts=[%d].",iOldFieldCounts,m_iFiledCounts);
                int i = 0;
                for(i = 0; i < m_iFiledCounts - iOldFieldCounts; ++i)
                {
                    m_vField.push_back(new TMdbClientField());
                }
                std::vector<TMdbClientField *>::iterator itorOld = m_vField.begin();
                TMdbClientField * pTempField = NULL;
                for(; itorNew != vColumnValue.end(); ++itorNew)
                {
                    pTempField = (*itorOld);
                    pTempField->ClearDataBuf();
                    pTempField->m_iDataType = DT_VarChar;
                    pTempField->m_sName 	= m_pCspSetSQLAns->GetStringValue((*itorNew)->pChildItem,AVP_COLUMN_NAME);
                    pTempField->m_bIsNULL	= m_pCspSetSQLAns->IsNullValue((*itorNew)->pChildItem,AVP_COLUMN_VALUE);
                    if(false == pTempField->m_bIsNULL)
                    {
                        pTempField->m_sValue    = m_pCspSetSQLAns->GetStringValue((*itorNew)->pChildItem,AVP_COLUMN_VALUE);
                    }
                    ++itorOld;
                    TADD_DETAIL("Field,name=[%s]:value=[%s].",pTempField->m_sName,pTempField->m_bIsNULL?"(nil)":pTempField->m_sValue);
                }
                m_pCurRowGroup = m_pCurRowGroup->pNextItem;
                m_pCurRowGroup = m_pCspSetSQLAns->FindExistAvpItem(m_pCurRowGroup,AVP_ROW_GROUP);
                TADD_DETAIL("m_pCurRowGroup=[%p].",m_pCurRowGroup);
            }
            else
            {
                iRet = -1;// û����һ����
            }
        }
        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish iRet=[%d].",iRet);
        return iRet;
    }

    //����������ȡ��i����ʵ��,��0��ʼ
    TMdbClientField& TMdbClientQuery::Field(int iIndex) throw (TMdbException)
    {
        if(iIndex < m_iFiledCounts)
        {
            return (*m_vField[iIndex]);
        }
        else
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"Input parameters Error,iIndex=[%d]>=m_iFiledCounts=[%d].",iIndex,m_iFiledCounts);
        }
    }

    //����������ȡ��ʵ��
    TMdbClientField& TMdbClientQuery::Field(const char *sFieldName) throw (TMdbException)
    {
        TADD_FUNC("FieldName=[%s].",sFieldName);
        int i = 0;
        for(i =0; i< m_iFiledCounts; ++i)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_vField[i]->m_sName,sFieldName) == 0)
                break;
        }
        if(i == m_iFiledCounts)
        {
            ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"Input parameters[%s] Error.",sFieldName);
        }
        return (*m_vField[i]);
    }

    /******************************************************************************
    * ��������	:  GetValue
    * ��������	:  ��ȡһ����¼
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  zhang.lin
    *******************************************************************************/
    void TMdbClientQuery::GetValue(void *pStruct,int* Column)throw (TMdbException)
    {
        TADD_FUNC("Start.");
        char* pStructAddr = (char*)pStruct;
        int i = 0;
        int iFiledCounts = FieldCount();
        char sValue[MAX_BLOB_LEN];
        //ȫ������Ϣ
        for(i = 0; i<iFiledCounts; ++i)
        {
            memset(sValue,0,sizeof(sValue));
            SAFESTRCPY(sValue,sizeof(sValue),Field(i).AsString());
            //printf("FIELD[%d] value= [%s] \n",i,sValue);
            //printf("test[%d] columnoffsetof [%d] \n",i,Column[i]);
            //ÿ�ж�Ӧ��filedֵ������ pStructAddr+Column[i]
            if(i == iFiledCounts - 1)//���һ��
            {
                //NOTICE:�г���Ϊ8������ֵ������ֵ���͵���ת�������һ��ֻ�ж��Ƿ��Ǵ���ֵ��
                if(TMdbNtcStrFunc::IsDigital(sValue))
                {
                    long long llValue = TMdbNtcStrFunc::StrToInt(sValue);
                    memcpy(pStructAddr+Column[i],(long long*)&llValue ,sizeof(long long));
                }
                else
                {
                    memcpy(pStructAddr+Column[i],sValue ,strlen(sValue));
                }

            }
            else
            {
                int iColLen = Column[i+1] - Column[i];
                //NOTICE:�г���Ϊ8������ֵ������ֵ���͵���ת����
                if(iColLen == 8 && TMdbNtcStrFunc::IsDigital(sValue))
                {
                    long long llValue = TMdbNtcStrFunc::StrToInt(sValue);
                    memcpy(pStructAddr+Column[i],(long long*)&llValue ,Column[i+1] - Column[i]);
                }
                else
                {
                    memcpy(pStructAddr+Column[i],sValue ,Column[i+1] - Column[i]);
                }
            }
        }
        TADD_FUNC("Finish.");
    }

    bool TMdbClientQuery::IsParamExist(const char *paramName)
    {
        return GetParamIndexByName(paramName) < 0?false:true;
    }

    /******************************************************************************
    * ��������	:  SetParameter
    * ��������	:  ���ò���ֵ
    * ����		:  sParamName - ������
    * ����		:  sParamValue - ����ֵ
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientQuery::SetParameter(const char *sParamName,const char* sParamValue, bool isOutput_Unused) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(m_tParam.GetParamIndexByName(sParamName),sParamValue);
            return;
        }
    
        TADD_FUNC("Name=[%s],Value=[%s].",sParamName,sParamValue);
        TMdbAvpItem * pItem = m_pCspSetParam->GetFreeAvpItem(m_pCspSetParam->m_pRootAvpItem,AVP_PARAM_GROUP);
        if(NULL == pItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"GetFreeAvpItem faild");
        }
        try
        {
            m_bIsParamArray = false;
            int iParamGroup = m_pCspSetParam->GetTotalLen();
            m_pCspSetParam->SetItemValue(pItem->pChildItem,AVP_PARAM_NAME,sParamName);
            m_pCspSetParam->SetItemValue(pItem->pChildItem,AVP_PARAM_VALUE,sParamValue);
            m_pCspSetParam->FinishFillGroup(pItem,m_pCspSetParam->GetTotalLen()-iParamGroup);
            //pItem->FinishFill();
        }
        catch(TMdbCSPException& e)
        {
            m_pCspSetParam->Clear();
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            m_pCspSetParam->Clear();
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.");
    }

    /******************************************************************************
    * ��������	:  SetParameter
    * ��������	:  ���ò���ֵ
    * ����		:  sParamName - ������
    * ����		:  sParamValue - ����ֵ
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientQuery::SetParameter(const char *paramName, const char cParamValue, bool isOutput_Unused) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(m_tParam.GetParamIndexByName(paramName),cParamValue);
            return;
        }
    
        TADD_FUNC("Name=[%s],Value=[%c].",paramName,cParamValue);
        char sTemp[2] = {0};
        sTemp[0] = cParamValue;
        SetParameter(paramName,sTemp);
        TADD_FUNC("Finish.");
    }

    void TMdbClientQuery::SetParameter(const char *sParamName,int iParamValue, bool isOutput_Unused) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(m_tParam.GetParamIndexByName(sParamName),iParamValue);
            return;
        }
    
        char sParamValue[32] = {0};
        sprintf(sParamValue, "%d", iParamValue);
        SetParameter(sParamName,sParamValue,isOutput_Unused);
    }


    void TMdbClientQuery::SetParameter(const char *sParamName,long lParamValue, bool isOutput_Unused) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(m_tParam.GetParamIndexByName(sParamName),lParamValue);
            return;
        }
    
        char sParamValue[32] = {0};
        sprintf(sParamValue, "%ld", lParamValue);
        SetParameter(sParamName,sParamValue,isOutput_Unused);
    }


    void TMdbClientQuery::SetParameter(const char *sParamName,double dParamValue, bool isOutput_Unused) throw (TMdbException)
    {
        ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"not support SetParameter double");
    }


    void TMdbClientQuery::SetParameter(const char *sParamName,long long llParamValue, bool isOutput_Unused) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(m_tParam.GetParamIndexByName(sParamName),llParamValue);
            return;
        }
    
        char sParamValue[32] = {0};
        sprintf(sParamValue, "%lld", llParamValue);
        SetParameter(sParamName,sParamValue,isOutput_Unused);
    }

    void TMdbClientQuery::SetParameter(const char *sParamName,const char* sParamValue,int iBufferLen, bool isOutput_Unused) throw (TMdbException)//���ڴ���BLOB/BINARY�����ֶ�
    {
        //ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"not support SetParameter[blob].");
        std::string encoded = Base::base64_encode(reinterpret_cast<const unsigned char*>(sParamValue),iBufferLen); 
        SetParameter(sParamName, encoded.c_str(), isOutput_Unused);
    }

    //NULLֵ�趨
    void TMdbClientQuery::SetParameterNULL(const char *sParamName) throw (TMdbException)     //���ò���Ϊ��
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterNULLBin(m_tParam.GetParamIndexByName(sParamName));
            return;
        }
    
        TADD_FUNC("Name=[%s].",sParamName);
        TMdbAvpItem * pItem = m_pCspSetParam->GetFreeAvpItem(m_pCspSetParam->m_pRootAvpItem,AVP_PARAM_GROUP);
        if(NULL == pItem)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"GetFreeAvpItem faild");
        }
        try
        {
            m_bIsParamArray = false;
            int iParamGroupLen = m_pCspSetParam->GetTotalLen();
            m_pCspSetParam->SetItemValue(pItem->pChildItem,AVP_PARAM_NAME,sParamName);
            m_pCspSetParam->SetItemValueNULL(pItem->pChildItem,AVP_PARAM_VALUE);
            m_pCspSetParam->FinishFillGroup(pItem,m_pCspSetParam->GetTotalLen()-iParamGroupLen);
            //pItem->FinishFill();
        }
        catch(TMdbCSPException& e)
        {
            m_pCspSetParam->Clear();
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            m_pCspSetParam->Clear();
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.");
    }

    //��
    void TMdbClientQuery::SetParameter(int iParamIndex,const char* sParamValue) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(iParamIndex,sParamValue);
            return;
        }
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        SetParameter(sParamName,sParamValue);
    }

    void TMdbClientQuery::SetParameter(int iParamIndex, const char cParamValue) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(iParamIndex,cParamValue);
            return;
        }
    
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        SetParameter(sParamName,cParamValue);
    }

    void TMdbClientQuery::SetParameter(int iParamIndex,int iParamValue) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(iParamIndex,iParamValue);
            return;
        }
    
        SetParameter(iParamIndex,(long long)iParamValue);
    }


    void TMdbClientQuery::SetParameter(int iParamIndex,long lParamValue) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(iParamIndex,lParamValue);
            return;
        }
        SetParameter(iParamIndex,(long long)lParamValue);
    }
    void TMdbClientQuery::SetParameter(int iParamIndex,double dParamValue) throw (TMdbException)
    {
        ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"not support SetParameter double");
    }


    void TMdbClientQuery::SetParameter(int iParamIndex,long long llParamValue) throw (TMdbException)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterBin(iParamIndex,llParamValue);
            return;
        }
    
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        SetParameter(sParamName,llParamValue);
    }

    void TMdbClientQuery::SetParameter(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException)//���ڴ���BLOB/BINARY�����ֶ�
    {
        //ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"not support SetParameter[blob].");
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        SetParameter(sParamName,sParamName,iBufferLen);
    }


    void TMdbClientQuery::SetParameterNULL(int iParamIndex) throw (TMdbException)     //���ò���Ϊ��
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            SetParameterNULLBin(iParamIndex);
            return;
        }
    
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        SetParameterNULL(sParamName);
    }


    //�����������ֵ
    void TMdbClientQuery::SetParamArray(const char *sParamName,char **asParamValue,int iInterval,
                                        int iElementSize,int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
    {
        int iIndex = m_tParam.GetParamIndexByName(sParamName);
        if(iIndex < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by name[%s]",sParamName);
        }
        SetParamArray(iIndex,asParamValue,iInterval,iElementSize,iArraySize,bOutput,bNullArr);
    }


    void TMdbClientQuery::SetParamArray(const char *sParamName,int *aiParamValue,int iInterval,
                                        int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
    {
        int iIndex = m_tParam.GetParamIndexByName(sParamName);
        if(iIndex < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by name[%s]",sParamName);
        }
        SetParamArray(iIndex,aiParamValue,iInterval,iArraySize,bOutput,bNullArr);
    }


    void TMdbClientQuery::SetParamArray(const char *sParamName,long *alParamValue,int iInterval,
                                        int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
    {
        int iIndex = m_tParam.GetParamIndexByName(sParamName);
        if(iIndex < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by name[%s]",sParamName);
        }
        SetParamArray(iIndex,alParamValue,iInterval,iArraySize,bOutput,bNullArr);
    }


    void TMdbClientQuery::SetParamArray(const char *sParamName,double *adParamValue,
                                        int iInterval,int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
    {
        ERROR_TO_THROW(ERR_SQL_NOT_SUPPORT_SETPARAM_ARRAY,m_sSQL,"not support SetParamArray");
    }


    void TMdbClientQuery::SetParamArray(const char *sParamName,long long *allParamValue,int iInterval,
                                        int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
    {
        int iIndex = m_tParam.GetParamIndexByName(sParamName);
        if(iIndex < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by name[%s]",sParamName);
        }
        SetParamArray(iIndex,allParamValue,iInterval,iArraySize,bOutput,bNullArr);
    }

    void TMdbClientQuery::SetBlobParamArray(const char *sParamName,char *sParamValue,int iBufferLen,
                                            int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)//���ڴ���BLOB/BINARY�����ֶ��������
    {
        ERROR_TO_THROW(ERR_SQL_NOT_SUPPORT_SETPARAM_ARRAY,m_sSQL,"not support SetBlobParamArray");
    }

    void TMdbClientQuery::SetParamArray(int iParamIndex,char **asParamValue,int iInterval,
                                        int iElementSize,int iArraySize,bool bOutput ,bool * bNullArr  ) throw (TMdbException)
    {
        //find
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        int iRet = 0;
        m_bIsParamArray = true;
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Str),
                        ERR_SQL_SETPARAMETER_TYPE,m_sSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Str);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_psValue[i] =  (char *)asParamValue + iElementSize * i;
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    void TMdbClientQuery::SetParamArray(int iParamIndex,int *aiParamValue,int iInterval,
                                        int iArraySize,bool bOutput ,bool * bNullArr ) throw (TMdbException)
    {
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        int iRet = 0;
        m_bIsParamArray = true;
        //find
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Int),
                        ERR_SQL_SETPARAMETER_TYPE,m_sSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Int);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_pllValue[i] = (MDB_INT64)*(aiParamValue + i);
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    void TMdbClientQuery::SetParamArray(int iParamIndex,long *alParamValue,int iInterval,
                                        int iArraySize,bool bOutput ,bool * bNullArr  ) throw (TMdbException)
    {
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        int iRet = 0;
        m_bIsParamArray = true;
        //find
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Int),
                        ERR_SQL_SETPARAMETER_TYPE,m_sSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Int);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_pllValue[i] = (MDB_INT64)*(alParamValue + i);
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    void TMdbClientQuery::SetParamArray(int iParamIndex,double *adParamValue,int iInterval,
                                        int iArraySize,bool bOutput,bool * bNullArr ) throw (TMdbException)
    {
        ERROR_TO_THROW(ERR_SQL_NOT_SUPPORT_SETPARAM_ARRAY,m_sSQL,"not support SetParamArray double");
    }
    void TMdbClientQuery::SetParamArray(int iParamIndex,long long *allParamValue,int iInterval,
                                        int iArraySize,bool bOutput,bool * bNullArr ) throw (TMdbException)
    {
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        int iRet = 0;
        m_bIsParamArray = true;
        //find
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Int),
                        ERR_SQL_SETPARAMETER_TYPE,m_sSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Int);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_pllValue[i] = *(allParamValue + i);
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    void TMdbClientQuery::SetBlobParamArray(int iParamIndex,char *sParamValue,int iBufferLen,
                                            int iArraySize,bool bOutput,bool * bNullArr ) throw (TMdbException)
    {
        ERROR_TO_THROW(ERR_SQL_NOT_SUPPORT_SETPARAM_ARRAY,m_sSQL,"not support SetBlobParamArray");
    }

    //������ݿ����,�д��쳣
    void TMdbClientQuery::CheckError(const char* sSql) throw (TMdbException) 
    {}

    //��ʹ��ocpЭ��
    void TMdbClientQuery::SetSQLBin(const char *sSqlStatement,int iSqlFlag ,int iPreFetchRows) throw (TMdbException)
    
    {
        TADD_FUNC("SQL=[%s],Flag=[%d],fetchRows=[%d].",sSqlStatement,iSqlFlag,iPreFetchRows);
        int iRet = 0;
        m_tSendDataParse.ResetParamFlag(false);
        m_iSetParamCount = 0;
        if(m_sSQL == NULL)
        {
            m_sSQL = new char[MAX_SQL_LEN];
            memset(m_sSQL, 0, MAX_SQL_LEN);
            m_iSQLBuffLen = MAX_SQL_LEN;
        }
        CloseSQL();
        int sqlLen = strlen(sSqlStatement);
        if(sqlLen >= m_iSQLBuffLen)
        {
            SAFE_DELETE(m_sSQL);
            m_sSQL = new char[sqlLen + 100];
            if(m_sSQL == NULL)
            {
                ERROR_TO_THROW(ERR_OS_NO_MEMROY,m_sSQL,"Mem Not Enough.");
            }
            memset(m_sSQL, 0, sqlLen + 100);
            m_iSQLBuffLen = sqlLen + 100;
        }
        
        SAFESTRCPY(m_sSQL,m_iSQLBuffLen,sSqlStatement);
        m_bIsDynamic = IsDynamicSQL(sSqlStatement);//�Ƿ��Ƕ�̬������߿���������
        //ȥ��ĩβ��;
        int ilen = strlen(m_sSQL);
        bool isFound = false;
        while(m_sSQL[ilen] == ';')
        {
            isFound = true;
            ilen--;
        }
        if(isFound == true)
            m_sSQL[ilen+1] = '\0';
        else
            m_sSQL[ilen] = '\0';
        TADD_DETAIL("SQL=[%s],m_bIsDynamic=[%d].",m_sSQL,m_bIsDynamic);
        try
        {
            m_iSQLType = iSqlFlag;
            m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_SEND_SQL,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            m_tSendDataParse.SetData(&m_iSQLLabel,sizeof(int));
            m_tSendDataParse.SetData((char*)m_sSQL,ilen+1);
            m_tSendDataParse.SetSize();
            //����sql���
            iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_tSendDataParse.GetSize());//stocket����д����
            if(iRet < 0)
            {
                ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"send(%s:%d) failed, SocketID is invalid!",m_pMdb->m_sIP, m_pMdb->m_iPort);
            }
            TADD_DETAIL("Send SetSQL OK.");
            m_pMdb->RecvPackage(CSP_APP_SEND_SQL,m_sRecvPackage,*m_pHead);
            if(m_pHead->iCmdCode == CSP_APP_SEND_SQL)
            {
                m_tRecvDataParse.InitDataSrc((char*)m_sRecvPackage);
                int sqllabel = 0;
                m_tRecvDataParse.GetData(&sqllabel,SIZE_MSG_AVP_HEAD,sizeof(int));
                if(sqllabel != m_iSQLLabel)
                {
                    ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"SQL label not equal,sqllabel[%d] != m_iSQLLabel[%d]",sqllabel,m_iSQLLabel);
                }
                m_pMdb->CheckAnsCode(m_tRecvDataParse,m_pHead->iAnsCodePos);
                m_tRecvDataParse.GetData(&m_iIsNeedNextOper,SIZE_MSG_AVP_HEAD+sizeof(int),sizeof(int));
                m_tRecvDataParse.GetData(&m_iRowsAff,SIZE_MSG_AVP_HEAD+2*sizeof(int),sizeof(int));
                
                if(m_bIsDynamic)
                {
                    //���Ƕ�̬sqlһ��Ҫ��Զ˻�ȡ
                    m_iIsNeedNextOper = 1;
                    m_bCanOpenAgain = true;
                }
                else
                {
                    m_bCanOpenAgain = false;//��̬SQL��setSQL��ʱ��ִ�й�һ����
                    m_iRowsCurNext = m_iRowsAff;
                    m_iCurPos = SIZE_MSG_AVP_HEAD + 3*sizeof(int);
                }
                m_iFiledCounts = 0;
                TADD_DETAIL("m_iIsNeedNextOper=[%d],m_iRowsAff=[%d].",m_iIsNeedNextOper,m_iRowsAff);
            }
        }
    
        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"TMdbCSPException [%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.");
    }

    int TMdbClientQuery::SendParamBin() throw (TMdbException)
    {
        int iRet = 0;
        if(false == m_bIsParamArray)
        {
            //����
            m_tSendDataParse.SetSize();
            iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_tSendDataParse.GetSize());//stocket����д����
            m_tSendDataParse.ResetParamFlag(false);
            m_iSetParamCount = 0;
        }
        else
        {//������
            int iBatchCount = m_pParamArray[0].m_iArraySize;
            int iParamCount  = m_tParam.GetCount();
            int i = 0;
            m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_SEND_PARAM_ARRAY,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            m_tSendDataParse.SetData(&m_iSQLLabel,sizeof(int)); //sql label
            m_tSendDataParse.SetData(&iBatchCount,sizeof(int));//count            
            m_tSendDataParse.SetData(&iParamCount,sizeof(int));//count            
            
            int iLen = 0;
            int iPackageLen = 0;
            try
            {
                for(i = 0; i < iBatchCount; ++i)
                {
                    int iParamPos;
                    for(iParamPos = 0; iParamPos < iParamCount; ++iParamPos)
                    {
                    
                        if(m_pParamArray[iParamPos].m_tNullArrWrap.IsNull(i))
                        {
                            //NULLֵ
                            iPackageLen = sizeof(int);
                            m_tSendDataParse.SetData(&iPackageLen,sizeof(int));
                        }
                        else
                        {
                            switch(m_pParamArray[iParamPos].m_iParamType)
                            {
                            case MEM_Int:
                                {
                                    iPackageLen = sizeof(int) + sizeof(long long);
                                    m_tSendDataParse.SetData(&iPackageLen,sizeof(int));
                                    m_tSendDataParse.SetData(&m_pParamArray[iParamPos].m_pllValue[i],sizeof(long long));
                                    break;
                                }
                            case MEM_Str:
                                {
                                    iLen = strlen(m_pParamArray[iParamPos].m_psValue[i]) + 1;
                                    iPackageLen = sizeof(int) + iLen;
                                    m_tSendDataParse.SetData(&iPackageLen,sizeof(int));
                                    m_tSendDataParse.SetData(m_pParamArray[iParamPos].m_psValue[i],iLen);
                                    break;
                                }
                            
                            default:
                                ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND,m_sSQL,"Param[%d]Type[%d] error.",i,
                                               m_pParamArray[i].m_iParamType);
                                break;
                            }
                        }
                    }
                }
                m_tSendDataParse.SetSize();
                iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_tSendDataParse.GetSize());//stocket����д����
                
            }
            catch(TMdbCSPException& e)
            {
                ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
            }
            catch(...)
            {
                ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
            }
            
        }
        return iRet;
    }

    void TMdbClientQuery::OpenBin(int prefetchRows) throw (TMdbException)
    {
    TADD_FUNC("Start.m_bIsDynamic=[%d],m_bCanOpenAgain = [%d].",m_bIsDynamic,m_bCanOpenAgain);
    int iRet = 0;
    try
    {
        m_bFirstNext = true;
        m_iFiledCounts = 0;
        //��̬sql �����ٴ�OPEN
        if(m_bCanOpenAgain)
        {
            iRet = SendParamBin();
            if(iRet < 0)
            {
                ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"Open(%s:%d):send()failed, SocketID is invalid!",m_pMdb->m_sIP, m_pMdb->m_iPort);
            }
            TADD_DETAIL("Send SetParam OK.");
            m_pMdb->RecvPackage(CSP_APP_SEND_SQL,m_sRecvPackage,*m_pHead);
            if(m_pHead->iCmdCode == CSP_APP_SEND_SQL)
            {
                m_tRecvDataParse.InitDataSrc((char*)m_sRecvPackage);
                int sqllabel = 0;
                m_tRecvDataParse.GetData(&sqllabel,SIZE_MSG_AVP_HEAD,sizeof(int));
                if(sqllabel != m_iSQLLabel)
                {
                    ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"SQL label not equal,sqllabel[%d] != m_iSQLLabel[%d]",sqllabel,m_iSQLLabel);
                }
                m_pMdb->CheckAnsCode(m_tRecvDataParse,m_pHead->iAnsCodePos);
                m_tRecvDataParse.GetData(&m_iIsNeedNextOper,SIZE_MSG_AVP_HEAD+sizeof(int),sizeof(int));
                m_tRecvDataParse.GetData(&m_iRowsCurNext,SIZE_MSG_AVP_HEAD+2*sizeof(int),sizeof(int));
                m_iRowsAff = m_iRowsCurNext;
                m_iCurPos = SIZE_MSG_AVP_HEAD+3*sizeof(int);
                
                TADD_DETAIL("m_iIsNeedNextOper=[%d],m_iRowsAff=[%d].",m_iIsNeedNextOper,m_iRowsAff);
            }
        }
        m_bCanOpenAgain = true;//�����ٴα�open
    }
    catch(TMdbException& e )
    {
        throw;
    }
    catch(TMdbCSPException& e)
    {
        ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
    }
    catch(...)
    {
        ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
    }
    TADD_FUNC("Finish.");
    }

    int TMdbClientQuery::NewMemory(char * &pData,int iSize)
    {
        if(pData == NULL)
        {
            pData = new(std::nothrow) char[iSize];
            if(pData == NULL)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"Mem Not Enough.");
            }
        }
        return 0;
        
    }

    bool TMdbClientQuery::NextBin()throw (TMdbException)
    {
        TADD_FUNC("Start.m_iIsNeedNextOper=[%d].",m_iIsNeedNextOper);
        int  iRet = 0;
        bool bRet = false;
        try
        {
            if(FillFieldValueBin() != 0)
            {
                //û��next ֵ��������Ҫ��Զ������ȡ
                if(m_iIsNeedNextOper == 0)
                {
                    //�Զ�Ҳ��ʾû��û��������
                    bRet =  false;
                    m_fLastRecordFlag = true;
                }
                else
                {
                    m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_NEXT,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
                    m_tSendDataParse.SetData(&m_iSQLLabel,sizeof(int));
                    m_tSendDataParse.SetSize();
                    iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_tSendDataParse.GetSize());//stocket����д����
                    if(iRet < 0)
                    {
                        ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"send() failed, SocketID is invalid!");
                    }
                    TADD_DETAIL("Send Next OK.");
                    m_pMdb->RecvPackage(CSP_APP_SEND_SQL,m_sRecvPackage,*m_pHead);
                    if(m_pHead->iCmdCode == CSP_APP_SEND_SQL)
                    {
                        m_tRecvDataParse.InitDataSrc((char*)m_sRecvPackage);
                        int sqllabel = 0;
                        m_tRecvDataParse.GetData(&sqllabel,SIZE_MSG_AVP_HEAD,sizeof(int));
                        if(sqllabel != m_iSQLLabel)
                        {
                            ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"SQL label not equal,sqllabel[%d] != m_iSQLLabel[%d]",sqllabel,m_iSQLLabel);
                        }
                        m_pMdb->CheckAnsCode(m_tRecvDataParse,m_pHead->iAnsCodePos);
                        m_tRecvDataParse.GetData(&m_iIsNeedNextOper,SIZE_MSG_AVP_HEAD+sizeof(int),sizeof(int));
                        m_tRecvDataParse.GetData(&m_iRowsCurNext,SIZE_MSG_AVP_HEAD+2*sizeof(int),sizeof(int));
                        m_iRowsAff += m_iRowsCurNext;
                        m_iCurPos   = SIZE_MSG_AVP_HEAD + 3*sizeof(int);
                        TADD_DETAIL("m_iIsNeedNextOper=[%d].",m_iIsNeedNextOper);
                        return NextBin();
                    }
                }
            }
            else
            {
                bRet =  true;
            }
        }
        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.ret=[%d].",bRet);
        return bRet;
    }
    
    /******************************************************************************
    * ��������  :  FillFieldValue
    * ��������  :  ����ʣ���rowgroup �����
    * ����      :
    * ����      :
    * ���      :
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����      :  jin.shaohua
    *******************************************************************************/
    int TMdbClientQuery::FillFieldValueBin()throw (TMdbException)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            if(m_iRowsCurNext-- > 0)
            {
                if(m_iFiledCounts <= 0)//ֻ��ȡһ��
                {
                    m_tRecvDataParse.GetData(&m_iFiledCounts,m_iCurPos,sizeof(int));
                    m_iCurPos += sizeof(int);
                }
                int iOldFieldCounts = m_vField.size();
                TADD_DETAIL("iOldFieldCounts=[%d],m_iFiledCounts=[%d].",iOldFieldCounts,m_iFiledCounts);
                int i = 0;
                for(i = 0; i < m_iFiledCounts - iOldFieldCounts; ++i)
                {
                    m_vField.push_back(new TMdbClientField());
                }
                std::vector<TMdbClientField *>::iterator itorOld = m_vField.begin();
                TMdbClientField * pTempField = NULL;
                //unsigned short int iNameLen = 0;
                int iValueLen = 0;
                //char cDataType = 0;
                for(i=0; i < m_iFiledCounts; ++i)
                {
                    pTempField = (*itorOld);
                    pTempField->ClearDataBufPlus();
                    pTempField->SetUseOcp(false);
                    //pTempField->m_iDataType = DT_VarChar;
                    if(m_bFirstNext)
                    {//ֻ��ȡһ��
                        m_tRecvDataParse.GetData(&pTempField->m_iDataType,m_iCurPos,sizeof(char));
                        m_iCurPos += sizeof(char);
                        m_tRecvDataParse.GetData(&iValueLen,m_iCurPos,sizeof(iValueLen));
                        m_iCurPos += sizeof(iValueLen);
                        NewMemory(pTempField->m_sName,MAX_AVP_NAME_LEN);
                        m_tRecvDataParse.GetData(pTempField->m_sName,m_iCurPos,iValueLen);
                        m_iCurPos += iValueLen;
                    }
                    m_tRecvDataParse.GetData(&iValueLen,m_iCurPos,sizeof(iValueLen));
                    m_iCurPos += sizeof(iValueLen);
                    //pTempField->m_sName   = m_pCspSetSQLAns->GetStringValue((*itorNew)->pChildItem,AVP_COLUMN_NAME);
                    pTempField->m_bIsNULL   = (0 == iValueLen);
                    if(false == pTempField->m_bIsNULL)
                    {
                        
                        if(DT_Int == pTempField->m_iDataType)
                        {
                            m_tRecvDataParse.GetData(&pTempField->m_iValue,m_iCurPos,iValueLen);
                        
                        }
                        else
                        {
                            NewMemory(pTempField->m_sValue,MAX_AVP_VALUE_LEN);
                            m_tRecvDataParse.GetData(pTempField->m_sValue,m_iCurPos,iValueLen);
                        }
                    }
                    ++itorOld;
                    m_iCurPos += iValueLen;
                    TADD_DETAIL("Field,name=[%s]:value=[%s].",pTempField->m_sName,pTempField->m_bIsNULL?"(nil)":pTempField->m_sValue);
                }
                m_bFirstNext = false;
                TADD_DETAIL("m_pCurRowGroup=[].");
            }
            else
            {
                //m_iFiledCounts = 0;
                iRet = -1;// û����һ����
            }
        }
        catch(TMdbException& e )
        {
            throw;
        }
        catch(TMdbCSPException& e)
        {
            ERROR_TO_THROW(ERR_CSP_PARSER_ERROR_CSP,m_sSQL,"[%d][%s].",e.GetErrCode(),e.GetErrMsg());
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish iRet=[%d].",iRet);
        return iRet;
    }

    void TMdbClientQuery::SetParameterBin(int iParamIndex,const char* sParamValue) throw (TMdbException)
    {
        if(iParamIndex < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"invalid index");
        }
    
        if(false == m_tSendDataParse.GetSetParamFlag())
        {
            m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_SEND_PARAM,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            m_tSendDataParse.SetData(&m_iSQLLabel,sizeof(int)); //sql label
            int iCount = ParamCount();
            m_tSendDataParse.SetData(&iCount,sizeof(int));//count            
            m_tSendDataParse.ResetParamFlag(true);
        }
        
        int iLen = strlen(sParamValue)+1;
        int iPackageLen = sizeof(int) + sizeof(int) + iLen;
        m_tSendDataParse.SetData(&iPackageLen,sizeof(int));
        m_tSendDataParse.SetData(&iParamIndex,sizeof(int));
        m_tSendDataParse.SetData((char*)sParamValue,iLen);
        m_iSetParamCount++;
    
    }
    
    void TMdbClientQuery::SetParameterBin(int iParamIndex, const char cParamValue) throw (TMdbException)
    {
        SetParameterBin(iParamIndex,(long long)cParamValue);
        
    }
    
    void TMdbClientQuery::SetParameterBin(int iParamIndex,int iParamValue) throw (TMdbException)
    {
        SetParameterBin(iParamIndex,(long long)iParamValue);
    }
    
    void TMdbClientQuery::SetParameterBin(int iParamIndex,long lParamValue) throw (TMdbException)
    {
        SetParameterBin(iParamIndex,(long long)lParamValue);
    }
    void TMdbClientQuery::SetParameterBin(int iParamIndex,double dParamValue) throw (TMdbException)
    {
        ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"not support SetParameter double");
    }
    
    
    void TMdbClientQuery::SetParameterBin(int iParamIndex,long long llParamValue) throw (TMdbException)
    {
        if(iParamIndex < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"invalid index");
        }
    
        if(false == m_tSendDataParse.GetSetParamFlag())
        {
            m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_SEND_PARAM,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            m_tSendDataParse.SetData(&m_iSQLLabel,sizeof(int)); //sql label
            int iCount = ParamCount();
            m_tSendDataParse.SetData(&iCount,sizeof(int));//count            
            m_tSendDataParse.ResetParamFlag(true);
        }
        //iPackageLen + iParamIndex + iParamValue
        int iPackageLen = sizeof(int) + sizeof(int) + sizeof(llParamValue);
        m_tSendDataParse.SetData(&iPackageLen,sizeof(int));
        m_tSendDataParse.SetData(&iParamIndex,sizeof(int));
        m_tSendDataParse.SetData(&llParamValue,sizeof(llParamValue));
        m_iSetParamCount++;
    }
    
    void TMdbClientQuery::SetParameterBin(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException)//���ڴ���BLOB/BINARY�����ֶ�
    {
        //ERROR_TO_THROW(ERR_SQL_INVALID,m_sSQL,"not support SetParameter[blob].");
        const char * sParamName = m_tParam.GetParamNameByIndex(iParamIndex);
        if(NULL ==sParamName)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"not find param by index[%d]",iParamIndex);
        }
        SetParameter(sParamName,sParamName,iBufferLen);
    }
    
    void TMdbClientQuery::SetParameterNULLBin(int iParamIndex) throw (TMdbException)     //���ò���Ϊ��
    {
        if(iParamIndex < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"invalid index");
        }
    
        if(false == m_tSendDataParse.GetSetParamFlag())
        {
            m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_SEND_PARAM,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            m_tSendDataParse.SetData(&m_iSQLLabel,sizeof(int)); //sql label
            int iCount = ParamCount();
            m_tSendDataParse.SetData(&iCount,sizeof(int));//count            
            m_tSendDataParse.ResetParamFlag(true);
        }
        //iPackageLen + iParamIndex + iParamValue
        int iPackageLen = sizeof(int) + sizeof(int);
        m_tSendDataParse.SetData(&iPackageLen,sizeof(int));
        m_tSendDataParse.SetData(&iParamIndex,sizeof(int));
        m_iSetParamCount++;
    }
    
    long long TMdbClientQuery::GetSequenceByNameBin(const char * sName)throw (TMdbException)
    {
        TADD_FUNC("Start.SeqName=[%s].",sName);
        int iRet = 0;
        long long llSeqValue = 0;
        try
        {
            m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_GET_SEQUENCE,m_pMdb->m_lSessionId,m_pMdb->GetSendSequence());
            m_tSendDataParse.SetData((char*)sName,strlen(sName)+1);
            m_tSendDataParse.SetSize();
            iRet = m_pMdb->m_pSocket->write(m_sSendPackage,m_tSendDataParse.GetSize());//stocket����д����
            if(iRet < 0)
            {
                ERROR_TO_THROW(ERR_NET_SEND_FAILED,m_sSQL,"send(%s:%d) failed, SocketID is invalid!",m_pMdb->m_sIP, m_pMdb->m_iPort);
            }
            TADD_DETAIL("Send SetSQL OK.");
            m_pMdb->RecvPackage(CSP_APP_GET_SEQUENCE,m_sRecvPackage,*m_pHead);
            if(m_pHead->iCmdCode == CSP_APP_GET_SEQUENCE)
            {
                m_tRecvDataParse.InitDataSrc((char*)m_sRecvPackage);
                m_tRecvDataParse.GetData(&llSeqValue,SIZE_MSG_AVP_HEAD,sizeof(int));
                m_pMdb->CheckAnsCode(m_tRecvDataParse,m_pHead->iAnsCodePos);
            }
        }
        catch(TMdbException& e )
        {
            throw;
        }
        catch(...)
        {
            ERROR_TO_THROW(ERROR_UNKNOWN,m_sSQL,"unknow error.");
        }
        TADD_FUNC("Finish.SeqValue=[%ld].",llSeqValue);
        return llSeqValue;
    }
    

    /******************************************************************************
    * ��������	:  TMdbClientDatabase
    * ��������	:  ����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbClientDatabase::TMdbClientDatabase()
    {
        memset(m_sUser, 0, sizeof(m_sUser));
        memset(m_sPWD, 0, sizeof(m_sPWD));
        memset(m_sDSN, 0, sizeof(m_sDSN));
        memset(m_sIP, 0, sizeof(m_sIP));
        memset(m_sLocalIP,0,sizeof(m_sLocalIP));
        m_bConnectFlag = false;
        m_iSocketID = -1;
        m_lSessionId = 0;
        m_iSQLFlag = 0;
        m_iPort = 0;
        m_iTimeout = 3;//��ʱʱ��Ĭ��Ϊ3��
        m_iSendSequence = 0;

        m_pMultiProtector = NULL;
        m_pCspParserMgr = NULL;
        m_pSocket = NULL;
        m_pHead = NULL;
        m_pMdbClientEngine = NULL;
        m_pCspLogOnSend = NULL;
        m_pCspLogOnRecv = NULL;
        m_pCspTransSend = NULL;
        m_pCspTransRecv = NULL;
        m_pCspErrorRecv = NULL;
        m_iIsBigEndian = 0;
    }

    /******************************************************************************
    * ��������	:  ~TMdbClientDatabase
    * ��������	:  ����
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbClientDatabase::~TMdbClientDatabase()
    {
        if(m_bConnectFlag == true)
        {
            //�Ͽ�����
            Disconnect();
        }
        SAFE_DELETE(m_pCspParserMgr);
        SAFE_DELETE(m_pHead);
        SAFE_DELETE(m_pSocket);
        SAFE_DELETE(m_pMultiProtector);
    }

    /******************************************************************************
    * ��������	:  SetServer
    * ��������	:  ����Զ�����ӵ�IP�Ͷ˿�
    * ����		:  pszIP - IP,
    * ����		:  pszPort - �˿�
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientDatabase::SetServer(const char* pszIP,int pszPort)
    {
        if(strlen(pszIP) < 32)
        {
            SAFESTRCPY(m_sIP,sizeof(m_sIP),pszIP);
            m_iPort = pszPort;
        }
    }
    /******************************************************************************
    * ��������	:  SetTimeout
    * ��������	:  ���ó�ʱʱ��
    * ����		:  iTimeout - ��ʱʱ��
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientDatabase::SetTimeout(int iTimeout)
    {
        if(iTimeout > 0)
        {
            m_iTimeout = iTimeout;
        }
    }


    /******************************************************************************
    * ��������	:  SetLogin
    * ��������	:  ���õ�½��Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientDatabase::SetLogin(const char *sUser,const char *sPassword,const char *sServerName) throw (TMdbException)
    {

        SAFESTRCPY(m_sUser,sizeof(m_sUser),sUser);
        SAFESTRCPY(m_sPWD,sizeof(m_sPWD),sPassword);
        SAFESTRCPY(m_sDSN,sizeof(m_sDSN),sServerName);
    }
    char*   TMdbClientDatabase::GetUser()
    {
        return m_sUser;   //��ȡuser,pwd,dsn
    }
    char*   TMdbClientDatabase::GetPWD()
    {
        return m_sPWD;
    };
    char*   TMdbClientDatabase::GetDSN()
    {
        return m_sDSN;
    };

    int TMdbClientDatabase::GetIP(char* sIP)
    {
        char szName[255] = {0};
        //��ȡ����������
        if(gethostname(szName, sizeof(szName)) == -1)
        {
            return -1;
        }
        struct hostent *hp = gethostbyname(szName);
        if(NULL == hp)
        {
            return -1;
        }
        strcpy(sIP, inet_ntoa(*(struct in_addr *)*(hp->h_addr_list)));
        return 0;
    }

    /******************************************************************************
    * ��������	:  GetPortAndIP
    * ��������	:  ��$QuickMDB_HOME/qmdb_inst_list.ini ��ȡip port
    * ����		:[DSN_NAME]
    * ����		:IP=10.4.5.20 #����Ǳ���������LocalHost�ַ�������
    * ����		:PORT=7008
    * ����ֵ	:
    * ����		:  wang.liebao
    *******************************************************************************/
    void TMdbClientDatabase::GetPortAndIP()
    {
        if(m_sIP[0] != 0) return ;//���֮ǰ�Ѿ����ù����Ͳ����ٶ�ȡ�����ļ���
        TMdbNtcReadIni ReadIni;
        char sFileName[MAX_PATH_NAME_LEN]={0};
        char sDsn[MAX_NAME_LEN]={0};
        snprintf(sFileName,sizeof(sFileName)-1,"%s/qmdb_inst_list.ini",getenv("QuickMDB_HOME"));
        if(false == ReadIni.OpenFile(sFileName))
        {
            ERROR_TO_THROW_NOSQL(ERR_OS_OPEN_FILE,"Open file[%s] failed.",sFileName);
        }
        
        SAFESTRCPY(sDsn,sizeof(sDsn),m_sDSN);
        TMdbNtcStrFunc::ToUpper(sDsn);//��дDSN
        TMdbNtcStringBuffer sItem = ReadIni.ReadString(sDsn,"IP","");
        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sItem.c_str(),"LocalHost"))
        {//����ֱ�Ӷ�ϵͳ����
            TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
            if(NULL == pConfig)
            {
                ERROR_TO_THROW_NOSQL(ERR_APP_INVALID_PARAM,"Get config[%s] failed.",sDsn);
            }
            SAFESTRCPY(m_sIP,sizeof(m_sIP),pConfig->GetDSN()->sLocalIP);
            m_iPort = pConfig->GetDSN()->iAgentPort[0];
        }
        else
        {//�Ǳ���
            SAFESTRCPY(m_sIP,sizeof(m_sIP),sItem.c_str());
            m_iPort = ReadIni.ReadInteger(sDsn,"PORT",0);
        }
        if(0 == m_sIP[0] || 0 == m_iPort)
        {
            ERROR_TO_THROW_NOSQL(ERR_APP_INVALID_PARAM,"Get IP PORT of DSN[%s] failed.",sDsn);
        }
        return ;
    }
    //ocp
    void TMdbClientDatabase::UseOcp()
    {
        SetUseOcpFlag(MDB_CS_USE_OCP);
    }
    /******************************************************************************
    * ��������	:  Connect
    * ��������	:  �������ݿ�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbClientDatabase::Connect(bool bIsAutoCommit) throw (TMdbException)
    {
        char sName[64];
        memset(sName, 0, sizeof(sName));
        snprintf(sName,sizeof(sName),"%s_%d",m_sDSN,TMdbOS::GetPID());
        TADD_OFFSTART(m_sDSN,sName,0,true);
        //��ʼ��������
        if(NULL == m_pCspParserMgr)
        {
            m_pCspParserMgr = new (std::nothrow) TMdbCspParserMgr();
            if(NULL == m_pCspParserMgr)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pCspParserMgr is NULL.");
            }
            
        }
        if(m_pCspLogOnSend == NULL)
        {
            m_pCspLogOnSend = m_pCspParserMgr->GetParserByType(CSP_APP_LOGON,true);
            if(m_pCspLogOnSend == NULL)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pCspLogOnSend is NULL");
            }
            
        }
        if(m_pCspLogOnRecv == NULL)
        {
            m_pCspLogOnRecv = m_pCspParserMgr->GetParserByType(CSP_APP_LOGON,false);
            if(m_pCspLogOnRecv == NULL)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pCspLogOnRecv is NULL");
            }
            
        }
        if(m_pCspErrorRecv == NULL)
        {
            m_pCspErrorRecv = m_pCspParserMgr->GetParserByType(CSP_APP_ERROR,false);
            if(m_pCspErrorRecv == NULL)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pCspErrorRecv is NULL");
            }
            
        }
        if(NULL == m_pHead)
        {
            m_pHead = new (std::nothrow) TMdbAvpHead();
            if(NULL == m_pHead)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pHead is NULL");
            }
        }
        m_pSocket = new (std::nothrow)Socket();
        if(NULL == m_pSocket)
        {
            ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pSocket is NULL");
        }

        
        if(m_sUser[0]==0 || m_sPWD[0]==0 || m_sDSN[0]==0)
        {
            ERROR_TO_THROW_NOSQL(ERR_NET_IP_INVALID,"Connect to %s/%s@%s failed. The parameter is invalid.",m_sUser, m_sPWD, m_sDSN);
        }
        TADD_DETAIL("Connect to %s/%s@%s.",m_sUser, m_sPWD, m_sDSN);
        int iRet = 0;
        //���ӵ������,����Socket
        GetPortAndIP();//�������ļ���ȡip /port
        iRet = LinkServer();
        if(iRet != 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_NET_PEER_REFUSE,"LinkServer[%s/%d][%s/%s@%s] error,error code = [%d]",m_sIP,m_iPort,m_sUser, m_sPWD, m_sDSN,iRet);
            return false;
        }
        if(MDB_CS_USE_OCP == m_iUseOcp)
        {
            if(m_pCspTransSend == NULL)
            {
                m_pCspTransSend = m_pCspParserMgr->GetParserByType(CSP_APP_ACTION,true);
                if(m_pCspTransSend == NULL)
                {
                    ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pCspTransSend is NULL");
                }
                
            }
            if(m_pCspTransRecv == NULL)
            {
                m_pCspTransRecv = m_pCspParserMgr->GetParserByType(CSP_APP_ACTION,false);
                if(m_pCspTransRecv == NULL)
                {
                    ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pCspTransRecv is NULL");
                }
                
            }
        }
        m_bConnectFlag = true;
        if(NULL == m_pMultiProtector)
        {
            m_pMultiProtector = new(std::nothrow)TMdbMultiProtector();
            if(NULL == m_pMultiProtector)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pMultiProtector is NULL");
            }
            
        }
        m_pMultiProtector->Reset();
        TADD_FUNC("Finish.");
        return true;
    }


    /******************************************************************************
    * ��������	:  Connect
    * ��������	:  �������ݿ�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbClientDatabase::Connect(const char *sUser, const char *sPassword, const char *sServerName, bool bIsAutoCommit) throw (TMdbException)
    {
        SAFESTRCPY(m_sUser,sizeof(m_sUser),sUser);
        SAFESTRCPY(m_sPWD,sizeof(m_sPWD),sPassword);
        SAFESTRCPY(m_sDSN,sizeof(m_sDSN),sServerName);
        return Connect(bIsAutoCommit);
    }
    /******************************************************************************
    * ��������	:  Disconnect
    * ��������	:  �Ͽ����ݿ�����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbClientDatabase::Disconnect() throw(TMdbException)
    {
        TADD_FUNC("Begin ");
        if(m_bConnectFlag == true)
        {
            m_pSocket->Close();
            TADD_DETAIL("m_oSocket Close ");
            m_iSocketID = -1;
            m_bConnectFlag = false;
            m_pMultiProtector->Reset();
            //--m_iSQLFlag;//sql��ǩ-1
        }
        TADD_FUNC("End ");
        return 0;
    }


    /******************************************************************************
    * ��������	:  LinkServer
    * ��������	:  ���ӵ������,����Socket
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbClientDatabase::LinkServerWithOtherPort()
    {
    	TADD_FUNC("Start.ip=[%s],port=[%d],timeout=[%d].",m_sIP,m_iPort,m_iTimeout);
        int iRet = 0;
        try
        {
            m_iSocketID = m_pSocket->connect(m_sIP,m_iPort,m_iTimeout);
            TADD_DETAIL("Client socket handle = [%d].",m_iSocketID);
            if(m_iSocketID  < 0)
            {
                m_bConnectFlag = false;
                return ERR_NET_PEER_INVALID;
            }
            //add time:20120509 ����������
            iRet = m_pSocket->SetBlock(m_iSocketID);
            if(iRet < 0)
            {
                CHECK_RET(iRet,"ERR_NET_SET_ATTR ");
            }
            //ƴд��¼��Ϣ����
            m_pCspLogOnSend->Clear();
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_USER_NAME,m_sUser);
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_USER_PWD,m_sPWD);
            char caProcessName[100];
            TMdbOS::GetProcessNameByPID(TMdbOS::GetPID(),caProcessName,sizeof(caProcessName));
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_PROCESS_NAME,caProcessName);
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_PROCESS_ID,(unsigned int)TMdbOS::GetPID());
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_THREAD_ID,(unsigned int)TMdbOS::GetTID());
			m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_LOW_PRIORITY,(unsigned int)0);//C++ Ĭ���޵����ȼ�����
            char name[20];
            gethostname(name,20);
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_ORIGIN_REALM,name);
#ifdef OS_SUN
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_OS_USER_NAME,cuserid(NULL));
#else
#ifdef WIN32
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_OS_USER_NAME,"windows");
#else
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_OS_USER_NAME,"linux");
#endif
#endif
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_TERMINAL_NAME,"Unix");

			//�ڶ��ε�½����ʱ��������ˣ��������˿��ˣ���������ѭ��
			 m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_CON_NUM,1);
            if(m_iIsBigEndian == 0)
            {
                m_iIsBigEndian = MdbNtcIsBigEndian()?(MDB_CS_BIG_ED):(MDB_CS_LIT_ED);
            }
            m_pCspLogOnSend->SetVersion(m_iIsBigEndian);
            m_pCspLogOnSend->Serialize(m_sSendPackage,m_lSessionId,GetSendSequence());

            //���͵�¼����
            iRet=m_pSocket->write(m_sSendPackage,m_pCspLogOnSend->m_tHead.iLen);//stocket����д����
            if(iRet < 0)
            {
                CHECK_RET(iRet,"Socket Write Error.");
            }
            iRet = iRet >0?0:iRet;
            TADD_DETAIL("Send LogOn OK.");
            RecvPackage(CSP_APP_LOGON,m_sRecvPackage,*m_pHead,m_pCspErrorRecv);
            m_lSessionId = m_pHead->iSessionId;
            m_iUseOcp = m_pHead->iVersion;
            if(CSP_APP_LOGON ==  m_pHead->iCmdCode)
            {
                //������¼��Ӧ����
                m_pCspLogOnRecv->DeSerialize(m_sRecvPackage,m_pHead->iLen);
                CheckAnsCode(m_pCspLogOnRecv);
			
            }

        }
        catch(TMdbException& e)
        {
            CHECK_RET(e.GetErrCode(),"[%d]%s",e.GetErrCode(),e.GetErrMsg());
        }
        catch(TMdbCSPException& e)
        {
            CHECK_RET(e.GetErrCode(),"%s",e.GetErrMsg());
        }
        catch(...)
        {
            CHECK_RET(-1,"unknow error.");
        }
        TADD_FUNC("Finish.");
        return iRet;

    }
	
    int TMdbClientDatabase::LinkServer()
    {
        TADD_FUNC("Start.ip=[%s],port=[%d],timeout=[%d].",m_sIP,m_iPort,m_iTimeout);
        int iRet = 0;
        try
        {
            m_iSocketID = m_pSocket->connect(m_sIP,m_iPort,m_iTimeout);
            TADD_DETAIL("Client socket handle = [%d].",m_iSocketID);
            if(m_iSocketID  < 0)
            {
                m_bConnectFlag = false;
                return ERR_NET_PEER_INVALID;
            }
            //add time:20120509 ����������
            iRet = m_pSocket->SetBlock(m_iSocketID);
            if(iRet < 0)
            {
                CHECK_RET(iRet,"ERR_NET_SET_ATTR ");
            }
            //ƴд��¼��Ϣ����
            m_pCspLogOnSend->Clear();
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_USER_NAME,m_sUser);
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_USER_PWD,m_sPWD);
            char caProcessName[100]="";
            TMdbOS::GetProcessNameByPID(TMdbOS::GetPID(),caProcessName,sizeof(caProcessName));
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_PROCESS_NAME,caProcessName);
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_PROCESS_ID,(unsigned int)TMdbOS::GetPID());
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_THREAD_ID,(unsigned int)TMdbOS::GetTID());
			m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_LOW_PRIORITY,(unsigned int)0);//C++ Ĭ���޵����ȼ�����
            char name[20] ={0};
            gethostname(name,20);
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_ORIGIN_REALM,name);
#ifdef OS_SUN
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_OS_USER_NAME,cuserid(NULL));
#else
#ifdef WIN32
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_OS_USER_NAME,"windows");
#else
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_OS_USER_NAME,"linux");
#endif
#endif
            m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_TERMINAL_NAME,"Unix");
			//��һ�ε�½
			 m_pCspLogOnSend->SetItemValue(m_pCspLogOnSend->m_pRootAvpItem,AVP_CON_NUM,0);
            if(m_iIsBigEndian == 0)
            {
                m_iIsBigEndian = MdbNtcIsBigEndian()?(MDB_CS_BIG_ED):(MDB_CS_LIT_ED);
            }
            m_pCspLogOnSend->SetVersion(m_iIsBigEndian);
            m_pCspLogOnSend->Serialize(m_sSendPackage,m_lSessionId,GetSendSequence());

            //���͵�¼����
            iRet=m_pSocket->write(m_sSendPackage,m_pCspLogOnSend->m_tHead.iLen);//stocket����д����
            if(iRet < 0)
            {
                CHECK_RET(iRet,"Socket Write Error.");
            }
            iRet = iRet >0?0:iRet;
            TADD_DETAIL("Send LogOn OK.");
            RecvPackage(CSP_APP_LOGON,m_sRecvPackage,*m_pHead,m_pCspErrorRecv);
            m_lSessionId = m_pHead->iSessionId;
            m_iUseOcp = m_pHead->iVersion;
            if(CSP_APP_LOGON ==  m_pHead->iCmdCode)
            {
                //������¼��Ӧ����
                int iPort = 0;
                m_pCspLogOnRecv->DeSerialize(m_sRecvPackage,m_pHead->iLen);
                CheckAnsCode(m_pCspLogOnRecv);
				if(NULL != m_pCspLogOnRecv->FindExistAvpItem(m_pCspLogOnRecv->m_pRootAvpItem,AVP_ANSWER_PORT))
					iPort = m_pCspLogOnRecv->GetINT32Value(m_pCspLogOnRecv->m_pRootAvpItem,AVP_ANSWER_PORT);
				//TADD_NORMAL("the port get from server is %d",iPort);
				if(iPort > 0)
				{
					m_pSocket->Close();
					m_iPort = iPort;
					m_lSessionId = 0;
					
					iRet = LinkServerWithOtherPort();

				}
            }
        }
        catch(TMdbException& e)
        {
            CHECK_RET(e.GetErrCode(),"[%d]%s",e.GetErrCode(),e.GetErrMsg());
        }
        catch(TMdbCSPException& e)
        {
            CHECK_RET(e.GetErrCode(),"%s",e.GetErrMsg());
        }
        catch(...)
        {
            CHECK_RET(-1,"unknow error.");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  TransBegin
    * ��������	:  ��������, ��ִ���κζ���
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientDatabase::TransBegin()
    {
        return;
    }

    /******************************************************************************
    * ��������	:  Commit
    * ��������	:  �ύ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/          																								//��������
    void TMdbClientDatabase::Commit()
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            MultCmdBin("Commit");
            return;
        }
        TADD_FUNC("Start.");
        m_pCspTransSend->Clear();
        m_pCspTransSend->SetItemValue(m_pCspTransSend->m_pRootAvpItem,AVP_COMMAND_NAME,"commit");
        m_pCspTransSend->Serialize(m_sSendPackage,m_lSessionId,GetSendSequence());
        if(m_pSocket->write(m_sSendPackage,m_pCspTransSend->m_tHead.iLen) < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_NET_SEND_FAILED,"send() failed, SocketID is invalid.");
        }
        RecvPackage(CSP_APP_ACTION,m_sRecvPackage,*m_pHead,m_pCspErrorRecv);
        if(CSP_APP_ACTION ==  m_pHead->iCmdCode)
        {
            //������Ӧ����
            m_pCspTransRecv->DeSerialize(m_sRecvPackage,m_pHead->iLen);
            CheckAnsCode(m_pCspTransRecv);
        }
        TADD_FUNC("Finish.");
    }

    /******************************************************************************
    * ��������	:  Rollback
    * ��������	:  �ع�����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientDatabase::Rollback()
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            MultCmdBin("Rollback");
            return;
        }
    
        TADD_FUNC("Start.");
        m_pCspTransSend->Clear();
        m_pCspTransSend->SetItemValue(m_pCspTransSend->m_pRootAvpItem,AVP_COMMAND_NAME,"rollback");
        m_pCspTransSend->Serialize(m_sSendPackage,m_lSessionId,GetSendSequence());
        if(m_pSocket->write(m_sSendPackage,m_pCspTransSend->m_tHead.iLen) < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_NET_SEND_FAILED,"send() failed, SocketID is invalid.");
        }
        RecvPackage(CSP_APP_ACTION,m_sRecvPackage,*m_pHead,m_pCspErrorRecv);
        if(CSP_APP_ACTION ==  m_pHead->iCmdCode)
        {
            //������Ӧ����
            m_pCspTransRecv->DeSerialize(m_sRecvPackage,m_pHead->iLen);
            CheckAnsCode(m_pCspTransRecv);
        }
        TADD_FUNC("Finish.");
    }

    /******************************************************************************
    * ��������	:  IsNullConnect
    * ��������	:  �Ƿ�Ϊ������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbClientDatabase::IsNullConnect()
    {
        return m_bConnectFlag;
    }

    /******************************************************************************
    * ��������	:  IsNullConnect
    * ��������	:  �Ƿ��Ѿ�����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbClientDatabase::IsConnect()
    {
        return m_bConnectFlag;
    }

    /******************************************************************************
    * ��������	:  CreateDBQuery
    * ��������	:  ����һ���µĲ�ѯ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbClientQuery *TMdbClientDatabase::CreateDBQuery() throw (TMdbException)
    {
        if(false == m_pMultiProtector->IsValid())
        {
            TADD_WARNING("current thread not connect,qmdb not support concurrency on one link.DBLink=[%d|%d],current=[%d|%lu]",
                m_pMultiProtector->GetPID(),m_pMultiProtector->GetTID(),TMdbOS::GetPID(),TMdbOS::GetTID());
            /*
             ERROR_TO_THROW_NOSQL(ERROR_DB_CURTHREAD_NOT_CONNECT,
                "current thread not connect,qmdb not support concurrency on one link.DBLink=[%d|%d],current=[%d|%lu]",
                m_pMultiProtector->GetPID(),m_pMultiProtector->GetTID(),TMdbOS::GetPID(),TMdbOS::GetTID());
                */
        }
        return new(std::nothrow) TMdbClientQuery(this,TMdbClientDatabase::m_iSQLFlag++);
    }

    void TMdbClientDatabase::CheckError(const char* sSql) throw (TMdbException) {}

    const char* TMdbClientDatabase::GetProvider()
    {
        return NULL;
    }

    /******************************************************************************
    * ��������	:  GetSQLFlag
    * ��������	:  ��ȡ������SQL Flag
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbClientDatabase::GetSQLFlag()
    {
        return TMdbClientDatabase::m_iSQLFlag;
    }

    /******************************************************************************
    * ��������	:  RecvPackage
    * ��������	:  ������Ϣ�������ж��Ƿ�����Ҫ�����Ͱ�
    * ����		:  iCspAppType - ϣ�����յİ����͡�
    * ����		:  sRecvMsg - ���յ�����Ϣ��
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientDatabase::RecvPackage(int iCspAppType,unsigned char * sRecvMsg,
                                         TMdbAvpHead & tAvpHead,TMdbCspParser * pCspErrorParser)throw (TMdbException)
    {
        TADD_FUNC("AppType=[%d].",iCspAppType);
        int iRet = 0;
        //���ջ�Ӧ
        iRet = m_pSocket->read(sRecvMsg,SIZE_MSG_AVP_HEAD);
        if(iRet < 0)
        {
            Disconnect();
            ERROR_TO_THROW_NOSQL(ERR_NET_RECV_FAILED,"Open(%s:%d):recv()failed, SocketID is invalid!",m_sIP,m_iPort);
        }
        sRecvMsg[SIZE_MSG_AVP_HEAD] = '\0';
        tAvpHead.Clear();
        tAvpHead.BinToCnvt(sRecvMsg);
        if(tAvpHead.sHeadName[5] == '>' && tAvpHead.sHeadName[0] == '<')
        {
            if((int)tAvpHead.iCmdCode == iCspAppType || tAvpHead.iCmdCode == CSP_APP_ERROR)
            {
                int iMsgLen = tAvpHead.iLen;
                iRet = m_pSocket->read(&sRecvMsg[SIZE_MSG_AVP_HEAD],iMsgLen-SIZE_MSG_AVP_HEAD);
                if(iRet < 0)
                {
                            Disconnect();
                            ERROR_TO_THROW_NOSQL(ERR_NET_RECV_FAILED,"Open(%s:%d):NTC client GetMessage()failed.",m_sIP,m_iPort);
                }
                IS_LOG(1)
                {
                    printf("\n------------------Open----begin------------------------------\n");
                    printf("iMsgLen = %d\n",iMsgLen);
                    for(int i=0; i<iMsgLen; ++i)
                    {
                        if(i%16 == 0)
                            printf("\n");
                        printf(" %02x", sRecvMsg[i]);
                        }
                    printf("\n");
                    printf("------------------Open----end------------------------------\n");
                }
            }
            else
            {
                ERROR_TO_THROW_NOSQL(ERR_CSP_PARSER_ERROR_CSP,"AVP CommonCode [%d]!= %d",tAvpHead.iCmdCode,iCspAppType);
            }
        }
        else
        {
            ERROR_TO_THROW_NOSQL(ERR_CSP_PARSER_ERROR_CSP,"Recved Head Package Error,Msg=[%s]",sRecvMsg);
        }
        if(tAvpHead.iCmdCode == CSP_APP_ERROR)
        {
            pCspErrorParser->DeSerialize(sRecvMsg,tAvpHead.iLen);
            CheckAnsCode(pCspErrorParser);
        }
        TADD_FUNC("Finish");
    }

    /******************************************************************************
    * ��������	:  CheckAnsCode
    * ��������	:  ���Ӧ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbClientDatabase::CheckAnsCode(TMdbCspParser * pParser)throw (TMdbException)
    {
        int iAnswerCode = pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_ANSWER_CODE);
        if(iAnswerCode != 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_CSP_PARSER_ERROR_CSP,"ErrorId=[%d] Error Msg=[%s].",iAnswerCode,
                                 pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_ANSWER_MSG));
        }
    }


    void TMdbClientDatabase::RecvPackage(int iCspAppType,unsigned char * sRecvMsg,
                                         TMdbAvpHead & tAvpHead)throw (TMdbException)
    {
        TADD_FUNC("AppType=[%d].",iCspAppType);
        int iRet = 0;
        //���ջ�Ӧ
        iRet = m_pSocket->read(sRecvMsg,SIZE_MSG_AVP_HEAD);
        if(iRet < 0)
        {
            Disconnect();
            ERROR_TO_THROW_NOSQL(ERR_NET_RECV_FAILED,"Open(%s:%d):recv()failed, SocketID is invalid!",m_sIP,m_iPort);
        }
        sRecvMsg[SIZE_MSG_AVP_HEAD] = '\0';
        tAvpHead.Clear();
        tAvpHead.BinToCnvtPlus(sRecvMsg);
        if(tAvpHead.sHeadName[5] == '>' && tAvpHead.sHeadName[0] == '<')
        {
            if((int)tAvpHead.iCmdCode == iCspAppType || tAvpHead.iCmdCode == CSP_APP_ERROR)
            {
                int iMsgLen = tAvpHead.iLen;
                iRet = m_pSocket->read(&sRecvMsg[SIZE_MSG_AVP_HEAD],iMsgLen-SIZE_MSG_AVP_HEAD);
                if(iRet < 0)
                {
                            Disconnect();
                            ERROR_TO_THROW_NOSQL(ERR_NET_RECV_FAILED,"Open(%s:%d):NTC client GetMessage()failed.",m_sIP,m_iPort);
                }
                IS_LOG(1)
                {
                    printf("\n------------------Open----begin------------------------------\n");
                    printf("iMsgLen = %d\n",iMsgLen);
                    for(int i=0; i<iMsgLen; ++i)
                    {
                        if(i%16 == 0)
                            printf("\n");
                        printf(" %02x", sRecvMsg[i]);
                        }
                    printf("\n");
                    printf("------------------Open----end------------------------------\n");
                }
            }
            else
            {
                ERROR_TO_THROW_NOSQL(ERR_CSP_PARSER_ERROR_CSP,"AVP CommonCode [%d]!= %d",tAvpHead.iCmdCode,iCspAppType);
            }
        }
        else
        {
            ERROR_TO_THROW_NOSQL(ERR_CSP_PARSER_ERROR_CSP,"Recved Head Package Error,Msg=[%s]",sRecvMsg);
        }
        if(tAvpHead.iCmdCode == CSP_APP_ERROR)
        {
            m_tRecvDataParse.InitDataSrc((char*)sRecvMsg);
            CheckAnsCode(m_tRecvDataParse,tAvpHead.iAnsCodePos);
        }
        TADD_FUNC("Finish");
    }

    void TMdbClientDatabase::CheckAnsCode(NoOcpParse &tRecvData,int iAnsCodePos)throw (TMdbException)
    {
        int iAnsCode = 0;
        tRecvData.GetData(&iAnsCode,iAnsCodePos,sizeof(int));
        if(iAnsCode != 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_CSP_PARSER_ERROR_CSP,"ErrorId=[%d] Error Msg=[%s].",iAnsCode,
                                 tRecvData.GetDataPtr() + iAnsCodePos + sizeof(int));
        }
    }

    void TMdbClientDatabase::MultCmdBin(const char *sCmd)
    {
        //const char *sCmd = "Commit";
        m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_ACTION,m_lSessionId,GetSendSequence());
        m_tSendDataParse.SetData((char*)sCmd,strlen(sCmd)+1);
        m_tSendDataParse.SetSize();
        int iRet = m_pSocket->write(m_sSendPackage,m_tSendDataParse.GetSize());//stocket����д����
        if(iRet < 0)
        {
            ERROR_TO_THROW(ERR_NET_SEND_FAILED,sCmd,"send(%s:%d) failed, SocketID is invalid!",m_sIP, m_iPort);
        }
        
        RecvPackage(CSP_APP_ACTION,m_sRecvPackage,*m_pHead);
        if(CSP_APP_ACTION ==  m_pHead->iCmdCode)
        {
            //������Ӧ����
            m_tRecvDataParse.InitDataSrc((char*)m_sRecvPackage);
            CheckAnsCode(m_tRecvDataParse,m_pHead->iAnsCodePos);
        }
    }


    /******************************************************************************
    * ��������	:  GetSendSequence
    * ��������	:  ��ȡ�������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  ���
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbClientDatabase::GetSendSequence()
    {
        if(m_iSendSequence <= 0 || m_iSendSequence > 1024*1024)
        {
            m_iSendSequence = 1;
        }
        else
        {
            m_iSendSequence ++;
        }
        return m_iSendSequence;
    }
    /******************************************************************************
    * ��������	:  GetQmdbInfo
    * ��������	:  ��ȡqmdb��Ϣ
    * ����		:sCmd ,sAnswer��Ϊjson��
    * ����		:
    * ���		:
    * ����ֵ	:  
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbClientDatabase::GetQmdbInfo(const char * sCmd,std::string & sAnswer)
    {
        if(MDB_CS_USE_BIN == m_iUseOcp)
        {
            return GetQmdbInfoBin(sCmd,sAnswer);
        }
    
        int iRet = 0;
        TADD_FUNC("Start.");
        TMdbCspParser * pCspParser = m_pCspParserMgr->GetParserByType(CSP_APP_QMDB_INFO,true);
        CHECK_OBJ(pCspParser);
        //����
        pCspParser->SetItemValue(pCspParser->m_pRootAvpItem,AVP_COMMAND_NAME,sCmd);
        pCspParser->Serialize(m_sSendPackage,m_lSessionId,GetSendSequence());
        if(m_pSocket->write(m_sSendPackage,pCspParser->m_tHead.iLen) < 0)
        {
            ERROR_TO_THROW_NOSQL(ERR_NET_SEND_FAILED,"send() failed, SocketID is invalid.");
        }
        //����
        pCspParser = m_pCspParserMgr->GetParserByType(CSP_APP_QMDB_INFO,false);
        RecvPackage(CSP_APP_QMDB_INFO,m_sRecvPackage,*m_pHead,m_pCspErrorRecv);
        if(CSP_APP_QMDB_INFO ==  m_pHead->iCmdCode)
        {
            //������Ӧ����
            pCspParser->DeSerialize(m_sRecvPackage,m_pHead->iLen);
            CheckAnsCode(pCspParser);
            sAnswer = pCspParser->GetStringValue(pCspParser->m_pRootAvpItem,AVP_ANSWER_MSG);//��ȡ����
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbClientDatabase::GetQmdbInfoBin(const char * sCmd,std::string & sAnswer)
    {
        //const char *sCmd = "Commit";
        m_tSendDataParse.SerializeHead((char*)m_sSendPackage,CSP_APP_QMDB_INFO,m_lSessionId,GetSendSequence());
        m_tSendDataParse.SetData((char*)sCmd,strlen(sCmd)+1);
        m_tSendDataParse.SetSize();
        int iRet = m_pSocket->write(m_sSendPackage,m_tSendDataParse.GetSize());//stocket����д����
        if(iRet < 0)
        {
            ERROR_TO_THROW(ERR_NET_SEND_FAILED,sCmd,"send(%s:%d) failed, SocketID is invalid!",m_sIP, m_iPort);
        }
        
        RecvPackage(CSP_APP_QMDB_INFO,m_sRecvPackage,*m_pHead);
        if(CSP_APP_QMDB_INFO ==  m_pHead->iCmdCode)
        {
            //������Ӧ����
            m_tRecvDataParse.InitDataSrc((char*)m_sRecvPackage);
            CheckAnsCode(m_tRecvDataParse,m_pHead->iAnsCodePos);
            sAnswer = m_tRecvDataParse.GetDataPtr() + m_pHead->iAnsCodePos + sizeof(int);
        }
        return iRet;
    }
    NoOcpParse::NoOcpParse()
    {
        m_pData     = NULL;
        m_pHead     = NULL;
        m_iSize     = 0;
        m_bSetParam = false;
    }
    NoOcpParse::~NoOcpParse()
    {
        SAFE_DELETE(m_pHead);
    }
    
    void NoOcpParse::InitDataSrc(char *pSrc)
    {
        if(pSrc)
        {
           m_pData = pSrc; 
           m_iSize = 0;
           if(m_pHead)
           {
               m_pHead->Clear();
           }
        }
    }
    
    void NoOcpParse::SerializeHead(char* pSrc,int iCmdCode,int SessionId,int iSequence)
    {
        InitDataSrc(pSrc);
        //if(InitDataSrc(pSrc) != 0) return -1;
        if(NULL == m_pHead)
        {
            m_pHead = new (std::nothrow) TMdbAvpHead();
            if(NULL == m_pHead)
            {
                ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pHead is NULL");
            }
        }
        
        m_pHead->Clear();
        m_pHead->SetCmdCode(iCmdCode);
        m_pHead->SetSequence(iSequence);
        m_pHead->CnvtToBinPlus((unsigned char*)pSrc,SessionId);
        m_iSize = SIZE_MSG_AVP_HEAD;
        return ;
    }
    
    
    void NoOcpParse::SetSize()
    {
        if(m_iSize > MAX_CSP_LEN)
        {
            ERROR_TO_THROW_NOSQL(ERR_CSP_PARSER_ERROR_CSP,"Msg size[%d] > MAX_SIZE[%d]",m_iSize,MAX_CSP_LEN);
        }
    
        memcpy(m_pData + 23,&m_iSize,sizeof(int));
        return ;
    }
    
    void NoOcpParse::SetData(char * pData,int iPos,int iLen)
    {
        memcpy(m_pData+iPos,pData,iLen);
        return ;
    }
    void NoOcpParse::SetData(int * pData,int iPos,int iLen)
    {
        memcpy(m_pData+iPos,(char*)pData,iLen);
        return ;
    }
    void NoOcpParse::SetData(int * pData,int iPos,size_t iLen)
    {
        memcpy(m_pData+iPos,(char*)pData,iLen);
        return ;
    }
    void NoOcpParse::SetData(long long * pData,int iLen,bool bAnsCode)
    {
        SetData((char*)pData,iLen,bAnsCode);
        return ;
    }
    void NoOcpParse::SetData(char * pData,int iLen,bool bAnsCode)
    {
        if(!pData) return ;
        char *pDataAddr = (char*)m_pData;
        pDataAddr += m_iSize;
        memcpy(pDataAddr,pData,iLen);
        if(bAnsCode)
        {
            memcpy(m_pData+10,&m_iSize,sizeof(int));//anscode λ��
        }
        m_iSize += iLen;
        return ;
    }

    void NoOcpParse::SetData(int * pData,int iLen,bool bAnsCode)
    {
        SetData((char*)pData,iLen,bAnsCode);
    }
    void NoOcpParse::SetData(unsigned short int * pData,int iLen,bool bAnsCode)
    {
        SetData((char*)pData,iLen,bAnsCode);
    }
    
    /*
    int NoOcpParse::SetData(int iData)
    {
        char *pDataAddr = (char*)m_pData;
        pDataAddr += m_iSize;
        memcpy(pDataAddr,&iData,sizeof(int));
        m_iSize += sizeof(int);
        
        return 0;
    }
    
    int NoOcpParse::SetData(char* pStr)
    {
        char *pDataAddr = (char*)m_pData;
        pDataAddr += m_iSize;
        int iLen = strlen(pStr)+1;
        memcpy(pDataAddr,pStr,iLen);
        m_iSize += iLen;
        
        return 0;
    }
    */
    void NoOcpParse::GetData(char * pData,int iPos,int iLen)
    {
        char *pDataAddr = (char*)m_pData;
        pDataAddr += iPos;
        memcpy(pData,pDataAddr,iLen);
        return ;
    }

    void NoOcpParse::GetData(int * pData,int iPos,int iLen)
    {
        GetData((char*)pData,iPos,iLen);
    }
    void NoOcpParse::GetData(unsigned short int * pData,int iPos,int iLen)
    {
        GetData((char*)pData,iPos,iLen);
    }
    void NoOcpParse::GetData(long long * pData,int iPos,int iLen)
    {
        GetData((char*)pData,iPos,iLen);
    }
    
    
