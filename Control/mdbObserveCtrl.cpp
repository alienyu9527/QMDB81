/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbObserve.cpp
*@Description�� mdb�۲��
*@Author:		jin.shaohua
*@Date��	    2012.10
*@History:
******************************************************************************************/
#include "Control/mdbObserveCtrl.h"
#include "Helper/mdbSQLParser.h"
#include "Helper/SyntaxTreeAnalyse.h"
#include "Control/mdbExecuteEngine.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"
#include "Helper/mdbOS.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    //�����۲���
    TObserveBase::TObserveBase():
        m_pShmDsn(NULL),
        m_pDsn(NULL),
        m_pObservePoint(NULL),
        m_pSqlParser(NULL),
        m_iLogFileSizeM(0),
        m_pExecEngine(NULL),
        m_pTempMem(NULL)
    {
        memset(m_sLogFile,0x00,sizeof(m_sLogFile));
       m_bObTable.clear();
    }
    TObserveBase::~TObserveBase()
    {
        SAFE_DELETE_ARRAY(m_pTempMem)
    }
    /******************************************************************************
    * ��������	:  Init
    * ��������	:  ��ʼ��
    * ����		:  sDsn - dsn��
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::Init(const char * sDsn,int iObserveType)
    {
        int iRet = 0;
        m_pShmDsn= TMdbShmMgr::GetShmDSN(sDsn);//��ȡDSN�����ڴ�
        CHECK_OBJ(m_pShmDsn);
        m_pDsn = m_pShmDsn->GetInfo();
        CHECK_OBJ(m_pDsn);
        m_pObservePoint  = m_pShmDsn->GetObPiontById(iObserveType);
        CHECK_OBJ(m_pObservePoint);
        ParseParam(m_pObservePoint->m_sParam);
        return iRet;
        //�趨��־��¼·��
    }

    /******************************************************************************
    * ��������	: Log
    * ��������	:  ��¼
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::Log(const char * fmt, ...)
    {
        int iRet = 0;
        if(NULL == m_pTempMem)
        {
            m_pTempMem = new char[40960];
        }
        CHECK_OBJ(m_pTempMem);
        char sCurDate[30] = {0};
        TMdbDateTime::GetCurrentTimeStr(sCurDate);
        sprintf(m_pTempMem,"%s [%d-%lu]|",sCurDate, TMdbOS::GetPID(),TMdbOS::GetTID());
        va_list ap;//��ʼ������ָ��
        va_start(ap, fmt);//��ʼ������ָ��
        vsprintf(m_pTempMem+strlen(m_pTempMem),fmt,ap);
        va_end(ap);//����
        //ĩβ��\n
        m_pTempMem[40960 - 10] = '\0';
        m_pTempMem[strlen(m_pTempMem) +1] = '\0';
        m_pTempMem[strlen(m_pTempMem)] = '\n';
        
        CHECK_RET(m_tCacheLog.Log(m_pTempMem),"m_tCacheLog.Log failed");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  bTerminalObserve
    * ��������	:  �Ƿ���ֹ�۲�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TObserveBase::bTerminalObserve()
    {
        if(NULL == m_pDsn)
        {
            return true;
        }
        if(m_pDsn->tCurTime.tv_sec <= m_pObservePoint->m_tTerminal.tv_sec)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    /******************************************************************************
    * ��������	:  SetSQLParser
    * ��������	: ����ִ������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::SetExecEngine(TMdbExecuteEngine * pExecEngine)
    {
        int iRet = 0;
        CHECK_OBJ(pExecEngine);
        m_pExecEngine = pExecEngine;
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetSQLParser
    * ��������	: ����SQL������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::SetSQLParser(TMdbSqlParser * pSqlParser)
    {
        int iRet = 0;
        CHECK_OBJ(pSqlParser);
        m_pSqlParser = pSqlParser;
        ParseParam(m_pObservePoint->m_sParam);//��Ҫ��������
        return iRet;
    }
    /******************************************************************************
    * ��������	:  StopObServe
    * ��������	: ֹͣ�۲�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::StopObServe()
    {
        int iRet = 0;
        CHECK_OBJ(m_pObservePoint);
        m_pObservePoint->m_tTerminal.tv_sec = 0;
        return iRet;
    }
    /******************************************************************************
    * ��������	:  GeneralParaseParam
    * ��������	: ͨ�ý�������
    * ����		:sParam - �������Ĳ���
    ���������µĲ���
    --filename={table_id}.txt      --filesize=16 --tableids=1,2,457  
    --filename={table_name}.txt --filesize=16 --tableids=1
    --filename={pid}-{tid}.txt     --filesize=16 --tableids=1,2,457  
    filesize��λ��M
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::GeneralParaseParam(const char * sParam)
    {
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(sParam,' ');
        const char * sPrefix[]={"--filename=","--filesize=","--flushcycle="};
        int iPrefixCount = 4;//��3��ǰ׺ 
        unsigned int i = 0;
        //������ǰ������
        CHECK_RET(SetLogFileName(""),"SetLogFileName() error");
        CHECK_RET(SetLogFileSize(""),"SetLogFileSize() error");
        CHECK_RET(SetFlushCycle("2|10"),"SetFlushCycle() error");
        
        for(i = 0; i< tSplit.GetFieldCount();++i)
        {
            if(NULL == tSplit[i] || 0 ==  tSplit[i][0])
            {continue;}
            int j = 0;
            for(j = 0;j < iPrefixCount;++j)
            {
                 if(strncmp(sPrefix[j],tSplit[i],strlen(sPrefix[j])) == 0)
                 {
                        break;
                 }
            }
            switch(j)
            {
                case 0:
                    CHECK_RET(SetLogFileName(tSplit[i] + strlen(sPrefix[j])),"SetLogFileName(%s)error.",tSplit[i]);
                    break;
                case 1:
                    CHECK_RET(SetLogFileSize(tSplit[i] + strlen(sPrefix[j])),"SetLogFileName(%s)error.",tSplit[i]);
                    break;
                case 2:
                    CHECK_RET(SetFlushCycle(tSplit[i] + strlen(sPrefix[j])),"SetFlushCycle(%s)error.",tSplit[i]);
                    break;
                default:
                    CHECK_RET(ERR_APP_INVALID_PARAM,"param [%s] error",tSplit[i]);
                    break;
            }
        }
        CHECK_RET(m_tCacheLog.SetLogFile(m_sLogFile,m_iLogFileSizeM),"m_tCacheLog.SetLogFile(%s,%d) failed",m_sLogFile,m_iLogFileSizeM);//�����ļ���С
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetFlushCycle
    * ��������	:  ��������ˢ�²���
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::SetFlushCycle(const char * sFlushCycleFormat)
    {
        int iRet = 0;
        CHECK_OBJ(sFlushCycleFormat);
        TMdbNtcSplit tSplit;
        tSplit.SplitString(sFlushCycleFormat,'|');
        if(2 != tSplit.GetFieldCount() || TMdbNtcStrFunc::IsDigital(tSplit[0]) == false ||
            false == TMdbNtcStrFunc::IsDigital(tSplit[1]) )
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"sFlushCycleFormat[%s] is error",sFlushCycleFormat);
        }
        CHECK_RET(m_tCacheLog.SetFlushCycle(TMdbNtcStrFunc::StrToInt(tSplit[0]),TMdbNtcStrFunc::StrToInt(tSplit[1])),
                "m_tCacheLog.SetFlushCycle error");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SetLogFileName
    * ��������	:  �����ļ���
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::SetLogFileName(const char * sFileNameFormat)
    {
        int iRet = 0;
        char sTemp[MAX_NAME_LEN] = {0};
        SAFESTRCPY(sTemp,sizeof(sTemp),sFileNameFormat);
        TMdbNtcStrFunc::Trim(sTemp);
        const char * sSubStr[]={"{table_name}","{pid}","{tid}"};
        int iSubStrCount = 4;
        char sSubStrReplaced[4][MAX_NAME_LEN] = {{0}};
        //�����滻ֵ
        if(NULL != m_pSqlParser )
        {
            sprintf(sSubStrReplaced[0],"%s",m_pSqlParser->m_stSqlStruct.pMdbTable->sTableName);
        }
        else
        {
            sprintf(sSubStrReplaced[0],"table_name");
        }
        sprintf(sSubStrReplaced[1],"%d",TMdbOS::GetPID());
        sprintf(sSubStrReplaced[2],"%lu",TMdbOS::GetTID());
        //�滻
        int i = 0;
        for(i = 0;i< iSubStrCount;++i)
        {
            //std::string sResult = TMdbNtcStrFunc::Replace(sTemp,sSubStr[i],sSubStrReplaced[i]);
            //SAFESTRCPY(sTemp,sizeof(sTemp),sResult.c_str());
            std::string sResult = TMdbNtcStrFunc::Replace(sTemp,sSubStr[i],sSubStrReplaced[i], sTemp);
            
        }
        //������־�ļ�
       sprintf(m_sLogFile,"%s/log/",getenv("QuickMDB_HOME"));
        if(TMdbNtcPathOper::IsExist(m_sLogFile)== false)
        {
            if(TMdbNtcDirOper::MakeFullDir(m_sLogFile) ==  false)
            {
                CHECK_RET(ERR_OS_NO_MEMROY,"mkdir(%s) error.",m_sLogFile);
            }
        }
        if(0 == m_pObservePoint->m_sName[0])
        {
            sprintf(m_pObservePoint->m_sName,"default_observe_%d",m_pObservePoint->m_iType); 
        }
        if(0 == sTemp[0])
        {
            sprintf(m_sLogFile+strlen(m_sLogFile),"/%s.txt",m_pObservePoint->m_sName);
        }
        else
        {
            sprintf(m_sLogFile+strlen(m_sLogFile),"/%s_%s",m_pObservePoint->m_sName,sTemp);
        }
        return iRet;
    }
    
    /******************************************************************************
    * ��������	: SetLogFileSize
    * ��������	:  �����ļ���С
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveBase::SetLogFileSize(const char * sFileSize)
    {
        int iRet = 0;
        char sTemp[MAX_NAME_LEN] = {0};
        SAFESTRCPY(sTemp,sizeof(sTemp),sFileSize);
        TMdbNtcStrFunc::Trim(sTemp);
        if(0 != sTemp[0])
        {//ʹ��Ĭ��ֵ
            if(TMdbNtcStrFunc::IsDigital(sTemp))
            {
                m_iLogFileSizeM = TMdbNtcStrFunc::StrToInt(sTemp);
                if(m_iLogFileSizeM < 16 || m_iLogFileSizeM > 1024*10)
                {
                    CHECK_RET(ERR_APP_INVALID_PARAM,"10G >= LogFileSize >= 16M ");
                }
            }
            else
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"sFileSize[%s] error.",sFileSize);
            }
        }
        if(m_iLogFileSizeM < 16){m_iLogFileSizeM = 16;}
        return iRet;
    }



    /******************************************************************************
    * ��������	:  InitObservePoint
    * ��������	:  ��ʼ���۲����Ϣ
    * ����		:  pArrObservePoint - �ڴ��й۲����ֵ��ַ
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveMgr::InitObservePoint(TShmList<TObservePoint> & ObList)
    {
        int iRet = 0;
        int i = 0;
        int iType[OB_END] = {OB_TABLE_EXEC};
        const char * sName[OB_END] = {"ob_table_exec"};
        for(i = 0; i<OB_END ; ++i)
        {
            TObservePoint tObPoint;
            tObPoint.m_iType = iType[i];
            SAFESTRCPY(tObPoint.m_sName,sizeof(tObPoint.m_sName),sName[i]);
            tObPoint.m_sParam[0] = 0;
            tObPoint.m_tTerminal.tv_sec = 0;
            ObList.push_back(tObPoint);
        }
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetObserveInst
    * ��������	:  ��ȡ�۲⹦����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TObserveBase * TObserveMgr::GetObserveInst(int iObserveType)
    {
        TObserveBase * pRetBase = NULL;
        switch(iObserveType)
        {
        case OB_TABLE_EXEC:
            pRetBase =  new TObserveTableExec();
            break;
        default:
            break;
        }
        return pRetBase;
    }

    /******************************************************************************
    * ��������	:  ShowAllObservePoint
    * ��������	:  ��ʾ���й۲����Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int  TObserveMgr::ShowAllObservePoint(const char * sDsn)
    {
        int iRet = 0;
        TMdbShmDSN * pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(pShmDsn);
        TADD_NORMAL("ObservePoint count=[%d].",OB_END);

        TShmList<TObservePoint >::iterator itor = pShmDsn->m_ObserverList.begin();
        for(;itor != pShmDsn->m_ObserverList.end();++itor)
        {
            TObservePoint * pPoint  = &(*itor);
            char sDate[30] = {0};
            TMdbDateTime::TimeToString(pPoint->m_tTerminal.tv_sec,sDate);
            TADD_NORMAL("[type = %d]:[name  = %s][endtime=%s][param = %s]",pPoint->m_iType,
                        pPoint->m_sName,sDate,pPoint->m_sParam);
        }
        return iRet;
    }

    /******************************************************************************
    * ��������	:  TObserveTableExec
    * ��������	:  �۲���SQLִ��
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TObserveTableExec::TObserveTableExec()
    {
        memset(&m_tOldTerminal,0,sizeof(m_tOldTerminal));
    }
    /******************************************************************************
    * ��������	:  ~TObserveTableExec
    * ��������	:  �۲���SQLִ��
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TObserveTableExec::~TObserveTableExec()
    {

    }
    /******************************************************************************
    * ��������	:  bNeedToObserve
    * ��������	:  �Ƿ���Ҫobserve
    * ����		:
    * ���		:
    * ����ֵ	:  true-��Ҫ�۲�false-����Ҫ�۲�
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TObserveTableExec::bNeedToObserve()
    {
        if(m_bObTable[m_pSqlParser->m_stSqlStruct.pMdbTable->sTableName])
        {
            //ֻ�е���Ҫ�۲�ı�
            return true;
        }
        else
        {
            return false;
        }
    }
    /******************************************************************************
    * ��������	:  ParseParam
    * ��������	:  ��������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveTableExec::ParseParam(const char * sParam)
    {
        int iRet = 0;
        if(NULL == sParam || strlen(sParam) > sizeof(m_pObservePoint->m_sParam))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"param[%p] is error.",sParam);
        }
        CHECK_RET(GeneralParaseParam(sParam),"GeneralParaseParam(%s) error",sParam);
        return iRet;
    }
    /******************************************************************************
    * ��������	:  Record
    * ��������	:  ��¼
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveTableExec::Record()
    {
        int iRet = 0;
        ReParseParam();
        if(!bTerminalObserve() && bNeedToObserve())
        {
            Log("SQL=[%s].Row_Affected=[%d]",m_pSqlParser->m_stSqlStruct.sSQL,m_pExecEngine->GetRowsAffected());
            std::vector<_ST_MEM_VALUE *> & arrMemValue = m_pSqlParser->m_listInputVariable.vMemValue;
            std::vector<_ST_MEM_VALUE *>::iterator itor = arrMemValue.begin();
            for(; itor != arrMemValue.end(); ++itor)
            {
                ST_MEM_VALUE * pstMemValue = *itor;
                Log("[%-10s],Flag=[%d],lvalue=[%lld],sValue=[%s],iSize=[%d]",
                        TSyntaxTreeAnalyse::ReplaceNull(pstMemValue->sAlias),
                        pstMemValue->iFlags,
                        pstMemValue->lValue,
                        TSyntaxTreeAnalyse::ReplaceNull(pstMemValue->sValue),
                        pstMemValue->iSize);            
            }
        }
        return iRet;
    }

    /******************************************************************************
    * ��������	:  ReParseParam
    * ��������	:  ���½�������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TObserveTableExec::ReParseParam()
    {
        if(m_tOldTerminal.tv_sec != m_pObservePoint->m_tTerminal.tv_sec)
        {
            m_tOldTerminal.tv_sec = m_pObservePoint->m_tTerminal.tv_sec;
            ParseParam(m_pObservePoint->m_sParam);
            m_tCacheLog.FlushImmediately();//����ˢ��
        }
        return 0;
    }

//}

