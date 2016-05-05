/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbChangeNotify.cpp
*@Description： 内存数据库的Oracle同步管理控制
*@Author:		li.shugang
*@Date：	    2009年5月07日
*@History:
******************************************************************************************/
#include "Dbflush/mdbChangeNotify.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbStruct.h"
#ifndef WIN32
#include <netdb.h>
#endif
#include <arpa/inet.h>
#include <netinet/in.h>

//namespace QuickMDB{

    #define MAX_REP_PREFETCH_ROWS 10000

    // 打开游标时间过长告警阀值
    #define MAX_OPEN_CURSOR_SEC 10

    CNQueryList::CNQueryList()
    {
        m_pMdbLink = NULL;
        m_pOraLink = NULL;
        m_pTable = NULL;
        m_pConfig = NULL;
        m_pQueryQ = NULL;
        m_pQueryU = NULL;
        m_pQueryI = NULL;
        m_pQueryD = NULL;
        m_iFlushCount = 0;
        memset(m_sUpperDsn,0,sizeof(m_sUpperDsn));
        memset(m_sOraSelectSQL,0,sizeof(m_sOraSelectSQL));
        memset(m_sOraDeleteSQL,0,sizeof(m_sOraDeleteSQL));
        memset(m_sGetSeqFromNotifySeqSQL,0,sizeof(m_sGetSeqFromNotifySeqSQL));
        memset(m_sGetSeqFromNotifySQL,0,sizeof(m_sGetSeqFromNotifySQL));
        memset(m_sInsertNotifySeqSQL,0,sizeof(m_sInsertNotifySeqSQL));
        memset(m_sUpdateNotifySeqSQL,0,sizeof(m_sUpdateNotifySeqSQL));
        memset(m_sSQL,0,sizeof(m_sSQL));
        memset(m_sChangeNotifyName,0,sizeof(m_sChangeNotifyName));
        memset(m_sChangeNotifySeqName,0,sizeof(m_sChangeNotifySeqName));
        memset(m_sShadowName, 0, sizeof(m_sShadowName));
    	
    }

    CNQueryList::~CNQueryList()
    {
    	SAFE_DELETE(m_pQueryQ);
    	SAFE_DELETE(m_pQueryU);
    	SAFE_DELETE(m_pQueryI);
    	SAFE_DELETE(m_pQueryD);
    }

    int CNQueryList::Init(TMdbDatabase *pMdbLink,TMDBDBInterface *pOraLink,TMdbTable* pTable,TMdbConfig  *pConfig,char* dsn_name,char* mdbChangeNotifyName,char* mdbChangeNotifySeqName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pMdbLink);
        CHECK_OBJ(pOraLink);
        CHECK_OBJ(pTable);
        CHECK_OBJ(pConfig);
        m_pMdbLink = pMdbLink;
        m_pOraLink = pOraLink;
        m_pTable = pTable;
        m_pConfig = pConfig;
        SAFESTRCPY(m_sUpperDsn, sizeof(m_sUpperDsn), TMdbNtcStrFunc::ToUpper(dsn_name));
        memset(m_sShadowName, 0, sizeof(m_sShadowName));
        if(true == pConfig->GetDSN()->m_bShadow)
        {
            snprintf(m_sShadowName, sizeof(m_sShadowName), "QMDB_SHADOW_%s",m_sUpperDsn);
        }
        SAFESTRCPY(m_sChangeNotifyName,sizeof(m_sChangeNotifyName),mdbChangeNotifyName);
        SAFESTRCPY(m_sChangeNotifySeqName,sizeof(m_sChangeNotifySeqName),mdbChangeNotifySeqName);
        CHECK_RET(InitMdbQuery(),"InitMdbQuery() Faild");
        CHECK_RET(InitOraSQL(),"InitOraQuery() Faild");
        TADD_FUNC("Finish.");
        return iRet;

    }

    int CNQueryList::InitMdbQuery()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        try
        {
            GetQuerySQL();
            m_pQueryQ = m_pMdbLink->CreateDBQuery();
            m_pQueryQ->SetSQL(m_sSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);

            //初始化Update
            // 列全为主键的表不支持update。
            if(0 == GetUpdateSQL())
            {
                m_pQueryU = m_pMdbLink->CreateDBQuery();
                CHECK_OBJ(m_pQueryU);
                m_pQueryU->SetSQL(m_sSQL,QUERY_NO_ORAFLUSH | QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
            }

            //初始化Insert
            m_pQueryI = m_pMdbLink->CreateDBQuery();
            CHECK_OBJ(m_pQueryI);
            GetInsertSQL();
            m_pQueryI->SetSQL(m_sSQL,QUERY_NO_ORAFLUSH | QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);

            //初始化Delete
            m_pQueryD = m_pMdbLink->CreateDBQuery();
            CHECK_OBJ(m_pQueryD);
            
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
            return ERROR_UNKNOWN;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
            return ERROR_UNKNOWN;
        }
        
        TADD_FUNC("Finish.");
        return iRet;

    }

    int CNQueryList::InitOraSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        GetOraSQL();
        GetOraDSQL();
        GetSeqFromNotifySeqSQL();
        GetSeqFromNotifySQL();
        GetInsertNotifySeqSQL();
        GetUpdateNotifySeqSQL();
        return iRet;
    }


    int CNQueryList::GetQuerySQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        memset(m_sSQL, 0, sizeof(m_sSQL));
        //首先拼写select部分
        SAFESTRCPY(m_sSQL,sizeof(m_sSQL),"select ");
        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            if(0 == i)
            {
                sprintf(m_sSQL, "%s %s", m_sSQL, m_pTable->tColumn[i].sName);
            }
            else
            {
                sprintf(m_sSQL, "%s, %s", m_sSQL, m_pTable->tColumn[i].sName);
            }
        }

        sprintf(m_sSQL, "%s from %s where ", m_sSQL, m_pTable->sTableName);

        //最后拼写主健字段
        for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
        {
            int iColumnNo = m_pTable->m_tPriKey.iColumnNo[i];
            if(i == 0)
            {
                sprintf(m_sSQL, "%s%s=:%s", m_sSQL, m_pTable->tColumn[iColumnNo].sName, m_pTable->tColumn[iColumnNo].sName);
            }
            else
            {
                sprintf(m_sSQL, "%s and %s=:%s", m_sSQL, m_pTable->tColumn[iColumnNo].sName, m_pTable->tColumn[iColumnNo].sName);
            }
        }
        TADD_NORMAL("m_sSQL=[%s],OperType=[%d]", m_sSQL,TK_SELECT);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int CNQueryList::GetUpdateSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        memset(m_sSQL, 0, sizeof(m_sSQL));
        // 所有列均为主键列，不支持update
        if(m_pTable->iColumnCounts == m_pTable->m_tPriKey.iColumnCounts)
        {
            TADD_NORMAL("Table[%s]:All Columns are Primary Keys.not support to update.", m_pTable->sTableName);
            return 1;
        }
        //首先拼写update的set部分
        sprintf(m_sSQL, "update %s set ", m_pTable->sTableName);
        bool bFirstFlag = false;
        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            if(true == isKey(i)){continue;}

            if(bFirstFlag == false)
            {
                sprintf(m_sSQL, "%s %s=:%s", m_sSQL, m_pTable->tColumn[i].sName, m_pTable->tColumn[i].sName);
                bFirstFlag = true;
            }
            else
            {
                sprintf(m_sSQL, "%s, %s=:%s", m_sSQL, m_pTable->tColumn[i].sName, m_pTable->tColumn[i].sName);
            }
        }

        sprintf(m_sSQL, "%s where ", m_sSQL);

        //最后拼写主健字段
        for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
        {
            int iColumnNo = m_pTable->m_tPriKey.iColumnNo[i];
            if(i == 0)
            {
                sprintf(m_sSQL, "%s%s=:%s%d", m_sSQL, m_pTable->tColumn[iColumnNo].sName, m_pTable->tColumn[iColumnNo].sName, iColumnNo);
            }
            else
            {
                sprintf(m_sSQL, "%s and %s=:%s%d", m_sSQL, m_pTable->tColumn[iColumnNo].sName, m_pTable->tColumn[iColumnNo].sName, iColumnNo);
            }
        }
        TADD_NORMAL("m_sSQL=[%s],OperType=[%d]", m_sSQL,TK_UPDATE);
        TADD_FUNC("Finish.");

        return iRet;
    }

    int CNQueryList::GetInsertSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        memset(m_sSQL, 0, sizeof(m_sSQL));
        //首先拼写insert部分
        sprintf(m_sSQL, "insert into %s (", m_pTable->sTableName);
        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            if(0 == i)
            {
                sprintf(m_sSQL, "%s%s", m_sSQL, m_pTable->tColumn[i].sName);
            }
            else
            {
                sprintf(m_sSQL, "%s, %s", m_sSQL, m_pTable->tColumn[i].sName);
            }
        }

        sprintf(m_sSQL, "%s) values (", m_sSQL);

        //拼写value部分
        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            if(0 == i)
            {
                sprintf(m_sSQL, "%s:%s", m_sSQL, m_pTable->tColumn[i].sName);
            }
            else
            {
                sprintf(m_sSQL, "%s, :%s", m_sSQL, m_pTable->tColumn[i].sName);
            }
        }

        sprintf(m_sSQL, "%s)", m_sSQL);
        TADD_NORMAL("m_sSQL=[%s],OperType=[%d]", m_sSQL,TK_INSERT);
        TADD_FUNC("Finish.");
        return iRet;
    }

    // 返回值大于0，表示sql不一致，需要setSQL
    // 返回值 =0 ，说明上次sql与此次sql一直，外层不需要再setSQL
    int CNQueryList::GetDeleteSQL(TMDBDBQueryInterface* ptOraQry)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(ptOraQry);
        bool bFirst = true;

        static char sTmpSQL[MAX_SQL_LEN] = {0};
        
        memset(sTmpSQL, 0, sizeof(sTmpSQL));
        sprintf(sTmpSQL, "delete from %s", m_pTable->sTableName);
        sprintf(sTmpSQL, "%s where ", sTmpSQL);

        //最后拼写主健字段
        for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
        {
            if(ptOraQry->Field(i).isNULL())
            {
                continue;
            }
            
            int iColumnNo = m_pTable->m_tPriKey.iColumnNo[i];
            if(bFirst)
            {
                sprintf(sTmpSQL, "%s%s=:%s", sTmpSQL, m_pTable->tColumn[iColumnNo].sName, m_pTable->tColumn[iColumnNo].sName);
                bFirst = false;
            }
            else
            {
                sprintf(sTmpSQL, "%s and %s=:%s", sTmpSQL, m_pTable->tColumn[iColumnNo].sName, m_pTable->tColumn[iColumnNo].sName);
            }
        }

        // 第一次构造sql or 本次构造的sql与之前sql条件不一样，需要setSQL
        if(0 != TMdbNtcStrFunc::StrNoCaseCmp(m_sSQL, sTmpSQL))
        {
            memset(m_sSQL, 0, sizeof(m_sSQL));
            SAFESTRCPY(m_sSQL, sizeof(m_sSQL), sTmpSQL);
            iRet = 1;
        }
        
        TADD_DETAIL("m_sSQL=[%s],OperType=[%d]", m_sSQL,TK_DELETE);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int CNQueryList::GetOraSQL()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        memset(m_sOraSelectSQL, 0, sizeof(m_sOraSelectSQL));
        if(m_pTable->m_sFlushSQL[0] != 0)
        {
            SAFESTRCPY(m_sOraSelectSQL,sizeof(m_sOraSelectSQL),m_pTable->m_sFlushSQL);
            if(true == m_pConfig->GetDSN()->m_bShadow)
            {
                TADD_DETAIL("Shadow mode ,repalce %s to %s", m_sChangeNotifyName, m_sShadowName);
                TADD_DETAIL("before repalce , m_sOraSelectSQL=[%s]", m_sOraSelectSQL);
                char sTmpSelSql[MAX_SQL_LEN] ={0};
                TMdbNtcStrFunc::Replace(m_sOraSelectSQL, m_sChangeNotifyName, m_sShadowName,sTmpSelSql);
                memset(m_sOraSelectSQL, 0, sizeof(m_sOraSelectSQL));
                SAFESTRCPY(m_sOraSelectSQL, sizeof(m_sOraSelectSQL), sTmpSelSql);
                TADD_DETAIL("after repalce , m_sOraSelectSQL=[%s]", m_sOraSelectSQL);
            }
        }
        else
        {
            SAFESTRCPY(m_sOraSelectSQL,sizeof(m_sOraSelectSQL),"select ");
            //拼写select字段
            for(int i=0; i<m_pTable->iColumnCounts; ++i)
            {
                if(0 == i)
                {
                    snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL)," A.%s", m_pTable->tColumn[i].sName);
                }
                else
                {
                    snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL),", A.%s", m_pTable->tColumn[i].sName);
                }
            }
            snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL),", B.TABLE_SEQUENCE, B.ACTION_TYPE,B.UPDATE_TIME ");
            //拼接where字段
            if(true == m_pConfig->GetDSN()->m_bShadow)
            {
                TADD_DETAIL("Shadow mode");
                snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL)," from %s A, %s B where B.TABLE_SEQUENCE >= :P_MIN_TABLE_SEQUANCE and B.TABLE_SEQUENCE <= :P_MAX_TABLE_SEQUANCE and B.table_name = '%s' and ",m_pTable->sTableName,m_sShadowName,m_pTable->sTableName);
            }
            else
            {
                snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL)," from %s A, %s B where B.TABLE_SEQUENCE >= :P_MIN_TABLE_SEQUANCE and B.TABLE_SEQUENCE <= :P_MAX_TABLE_SEQUANCE and B.table_name = '%s' and ",m_pTable->sTableName,m_sChangeNotifyName,m_pTable->sTableName);
            }
            

            for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
            {
                int iColumnNo = m_pTable->m_tPriKey.iColumnNo[i];
                if(i == (m_pTable->m_tPriKey.iColumnCounts -1))
                {
                    if(m_pTable->tColumn[iColumnNo].iDataType == DT_DateStamp)
                    {
                        snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL), " A.%s=to_date(B.key%d,'yyyy-mm-dd hh24:mi:ss') ",m_pTable->tColumn[iColumnNo].sName,i+1);
                    }
                    else
                    {
                        snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL), " A.%s=B.key%d ",m_pTable->tColumn[iColumnNo].sName,i+1);
                    }
                }
                else
                {
                    if(m_pTable->tColumn[iColumnNo].iDataType == DT_DateStamp)
                    {
                        snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL), " A.%s=to_date(B.key%d,'yyyy-mm-dd hh24:mi:ss') and ",m_pTable->tColumn[iColumnNo].sName,i+1);
                    }
                    else
                    {
                        snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL)," A.%s=B.key%d and ",m_pTable->tColumn[iColumnNo].sName,i+1);
                    }
                }
            }
            snprintf(m_sOraSelectSQL+strlen(m_sOraSelectSQL),sizeof(m_sOraSelectSQL), " order by table_sequence");
        }
        TADD_NORMAL("m_sOraSelectSQL=[%s],OperType=[%d]", m_sOraSelectSQL,TK_SELECT);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int CNQueryList::GetOraDSQL()
    {
        TADD_FUNC("Stat.");
        memset(m_sOraDeleteSQL, 0, sizeof(m_sOraDeleteSQL));
        SAFESTRCPY(m_sOraDeleteSQL,sizeof(m_sOraDeleteSQL),"select ");

        for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
        {
            snprintf(m_sOraDeleteSQL+strlen(m_sOraDeleteSQL),sizeof(m_sOraDeleteSQL), " key%d, ",i+1);
        }
        if(true == m_pConfig->GetDSN()->m_bShadow)
        {
            snprintf(m_sOraDeleteSQL+strlen(m_sOraDeleteSQL),sizeof(m_sOraDeleteSQL), " TABLE_SEQUENCE,UPDATE_TIME,ACTION_TYPE from  %s where TABLE_SEQUENCE >= :P_MIN_TABLE_SEQUANCE and TABLE_SEQUENCE <= :P_MAX_TABLE_SEQUANCE and table_name = '%s' and ACTION_TYPE = 'D' order by TABLE_SEQUENCE",m_sShadowName,m_pTable->sTableName);
        }
        else
        {
            snprintf(m_sOraDeleteSQL+strlen(m_sOraDeleteSQL),sizeof(m_sOraDeleteSQL), " TABLE_SEQUENCE,UPDATE_TIME,ACTION_TYPE from  %s where TABLE_SEQUENCE >= :P_MIN_TABLE_SEQUANCE and TABLE_SEQUENCE <= :P_MAX_TABLE_SEQUANCE and table_name = '%s' and ACTION_TYPE = 'D' order by TABLE_SEQUENCE",m_sChangeNotifyName,m_pTable->sTableName);
        }
        
        TADD_NORMAL("m_sOraDeleteSQL=[%s],OperType=[%d]", m_sOraDeleteSQL,TK_DELETE);
        TADD_FUNC("Finish.");
        return 0;
    }

    int CNQueryList::GetSeqFromNotifySeqSQL()
    {
        TADD_FUNC("Stat.");
    	memset(m_sGetSeqFromNotifySeqSQL, 0, sizeof(m_sGetSeqFromNotifySeqSQL));
    	sprintf(m_sGetSeqFromNotifySeqSQL, "select TABLE_SEQUENCE from %s where ip='%s' and lower(dsn_name)=lower('%s') and table_name = '%s'",m_sChangeNotifySeqName,m_pConfig->GetDSN()->sLocalIP,m_pConfig->GetDSN()->sName,m_pTable->sTableName);
        TADD_NORMAL("m_sGetSeqFromNotifySeqSQL=[%s]", m_sGetSeqFromNotifySeqSQL);
        TADD_FUNC("Finish.");
    	return 0;
    }

    int CNQueryList::GetSeqFromNotifySQL()
    {
        TADD_FUNC("Stat.");
    	memset(m_sGetSeqFromNotifySQL, 0, sizeof(m_sGetSeqFromNotifySQL));
        if(true == m_pConfig->GetDSN()->m_bShadow)
        {
        	#ifdef DB_ORACLE
            sprintf(m_sGetSeqFromNotifySQL, "select nvl(max(TABLE_SEQUENCE),-1) MAX_TABLE_SEQUENCE,nvl(min(TABLE_SEQUENCE),-1) MIN_TABLE_SEQUENCE from %s where table_name = '%s'",
                                m_sShadowName,m_pTable->sTableName);
			#elif DB_MYSQL
			sprintf(m_sGetSeqFromNotifySQL, "select ifnull(A.MAX_TABLE_SEQ, -1) MAX_TABLE_SEQUENCE, ifnull(B.MIN_TABLE_SEQ, -1) MIN_TABLE_SEQUENCE from (select max(TABLE_SEQUENCE) MAX_TABLE_SEQ from %s where table_name = '%s' order by TABLE_SEQUENCE desc limit 1) as A, (select min(TABLE_SEQUENCE) MIN_TABLE_SEQ from %s where table_name = '%s' order by TABLE_SEQUENCE asc limit 1) as B",
                                m_sShadowName,m_pTable->sTableName,m_sShadowName,m_pTable->sTableName);
			#endif
        }
        else
        {
        	#ifdef DB_ORACLE
            sprintf(m_sGetSeqFromNotifySQL, "select nvl(max(TABLE_SEQUENCE),-1) MAX_TABLE_SEQUENCE,nvl(min(TABLE_SEQUENCE),-1) MIN_TABLE_SEQUENCE from %s where table_name = '%s'",
                                m_sChangeNotifyName,m_pTable->sTableName);
			#elif DB_MYSQL
			sprintf(m_sGetSeqFromNotifySQL, "select ifnull(A.MAX_TABLE_SEQ, -1) MAX_TABLE_SEQUENCE, ifnull(B.MIN_TABLE_SEQ, -1) MIN_TABLE_SEQUENCE from (select max(TABLE_SEQUENCE) MAX_TABLE_SEQ from %s where table_name = '%s' order by TABLE_SEQUENCE desc limit 1) as A, (select min(TABLE_SEQUENCE) MIN_TABLE_SEQ from %s where table_name = '%s' order by TABLE_SEQUENCE asc limit 1) as B",
                                m_sChangeNotifyName,m_pTable->sTableName,m_sChangeNotifyName,m_pTable->sTableName);
			#endif
        }
        
                    
        TADD_NORMAL("m_sGetSeqFromNotifySQL=[%s]", m_sGetSeqFromNotifySQL);
        TADD_FUNC("Finish.");
    	return 0;
    }

    int CNQueryList::GetInsertNotifySeqSQL()
    {
        TADD_FUNC("Stat.");
    	memset(m_sInsertNotifySeqSQL, 0, sizeof(m_sInsertNotifySeqSQL));
        sprintf(m_sInsertNotifySeqSQL, " insert into %s(TABLE_SEQUENCE,ip, dsn_name, update_time,table_name) values(:TABLE_SEQUENCE,'%s','%s',sysdate,'%s') ",m_sChangeNotifySeqName,m_pConfig->GetDSN()->sLocalIP,m_sUpperDsn,m_pTable->sTableName);
        TADD_NORMAL("m_sInsertNotifySeqSQL=[%s]", m_sInsertNotifySeqSQL);
        TADD_FUNC("Finish.");
    	return 0;
    }

    int CNQueryList::GetUpdateNotifySeqSQL()
    {
        TADD_FUNC("Stat.");
    	memset(m_sUpdateNotifySeqSQL, 0, sizeof(m_sUpdateNotifySeqSQL));
        sprintf(m_sUpdateNotifySeqSQL, "update %s set update_time = to_date(:update_time,'yyyy-mm-dd hh24:mi:ss'), TABLE_SEQUENCE = :TABLE_SEQUENCE where ip='%s' and lower(dsn_name)=lower('%s') and table_name = '%s'",m_sChangeNotifySeqName,m_pConfig->GetDSN()->sLocalIP,m_sUpperDsn,m_pTable->sTableName);
        TADD_NORMAL("m_sUpdateNotifySeqSQL=[%s]", m_sUpdateNotifySeqSQL);
        TADD_FUNC("Finish.");
    	return 0;
    }

    bool CNQueryList::isKey(int iColumnNo)
    {
        for(int i = 0; i<m_pTable->m_tPriKey.iColumnCounts; i++)
        {
            int iCPos = m_pTable->m_tPriKey.iColumnNo[i];
            if(iCPos == iColumnNo){return true;}
        }
    	return false;
    }
    TMdbChangeNotify::TMdbChangeNotify()
    {
        m_pOraQry = NULL;
        m_pDBLink    = NULL;  //链接
    	m_mdbLink = NULL;
        m_pConfig    = NULL;

        m_pShmDSN   = NULL;
        m_pDsn      = NULL;
        m_iTotalFlushCount = 0;
        m_iTimeInterval = 5;
        m_iDelayTime = 60;
        memset(m_sOpenStartTime, 0, sizeof(m_sOpenStartTime));
        memset(m_sOpenEndTime, 0, sizeof(m_sOpenEndTime));
        memset(m_sQrySql, 0, sizeof(m_sQrySql));

        m_bAlloc =false;
        m_lFlushCnt = 0;
        m_lStartSeq = 0;
        m_lEndSeq = 0;
        m_pFlushList = NULL;
        memset(m_cFlushList, 0, sizeof(m_cFlushList));
        for(int i =0; i<MAX_TABLE_COUNTS; i++)
        {
            m_tQueryList[i] = NULL;
        }
    	memset(m_sChangeNotifyName,0,sizeof(m_sChangeNotifyName));
    	memset(m_sChangeNotifySeqName,0,sizeof(m_sChangeNotifySeqName));
    	memset(m_sShadowName, 0, sizeof(m_sShadowName));
		m_bNeedFlush = true;
    }


    TMdbChangeNotify::~TMdbChangeNotify()
    {
        ClearAllTableQuery();
        if(m_pDBLink != NULL)
        {
            m_pDBLink->Disconnect();
            SAFE_DELETE(m_pDBLink);
        }
    	SAFE_DELETE(m_mdbLink);
        SAFE_DELETE(m_pFlushList);
    	
    }

    /******************************************************************************
    * 函数名称	:  Init()
    * 函数描述	:  初始化：链接Oracle
    * 输入		:  pszDSN, 锁管理区所属的DSN
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回负数
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbChangeNotify::Init(const char* pszDSN, const char* pszName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(pszDSN == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"pszDSN == NULL.");
            return ERR_APP_INVALID_PARAM;
        }
        //构造配置对象
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        //连接上共享内存
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);

        m_iTimeInterval = m_pConfig->GetDSN()->m_iOraRepInterval;
        TADD_NORMAL("ORACLE2MDB ,rep time interval=%d", m_iTimeInterval);
        
        m_iDelayTime = m_pConfig->GetDSN()->m_iOraRepDelaySec;
        TADD_NORMAL("ORACLE2MDB ,Delay node time =%d", m_iDelayTime);

        // 构造获取是否有待刷新数据的sql
        
        //连接内存数据库
        int iPos = GetAdministrator();
    	if(iPos < 0){TADD_ERROR(ERROR_UNKNOWN,"No administrator.");return -1;}
        try
        {
        	if(m_mdbLink == NULL)
    		{
    			m_mdbLink = new(std::nothrow) TMdbDatabase();
    			CHECK_OBJ(m_mdbLink);
    		}
            m_mdbLink->SetLogin(m_pConfig->GetUser(iPos)->sUser, m_pConfig->GetUser(iPos)->sPwd, pszDSN);
            m_mdbLink->Connect();
            m_mdbLink->DisableCommit();

        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
            return ERR_DB_NOT_CONNECTED;
        }
        catch(...)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"Unknown error!\n");
            return ERR_DB_NOT_CONNECTED;
        }
        CHECK_RET(m_tProcCtrl.Init(pszDSN),"m_tProcCtrl failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }

    bool TMdbChangeNotify::isKey(int iColumnNo)
    {
        for(int i = 0; i<m_tCurTable->m_tPriKey.iColumnCounts; i++)
        {
            int iCPos = m_tCurTable->m_tPriKey.iColumnNo[i];
            if(iCPos == iColumnNo){return true;}
        }
    	return false;
    }

    int TMdbChangeNotify::GetAdministrator()
    {
    	int i = 0;
        for (i = 0; i < m_pConfig->GetUserCounts(); i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pConfig->GetUser(i)->sAccess,"Administrator") == 0)
            {
            	return i;
            }
        }
    	return -1;
    }


    /******************************************************************************
    * 函数名称  :  UpdateNotifySeqTable()
    * 函数描述  :  更新MDB_CHANGE_NOTIFY_SEQ表
    * 输入      :  pTable 表对象
    * 输出      :  无
    * 返回值    :  0 成功；非0:失败
    * 作者      :  cao.peng
    *******************************************************************************/
    int TMdbChangeNotify::UpdateNotifySeqTable(MDB_INT64 iUpdateSequence,char *sUpdateTime)
    {
        int iRet = 0;
    	TADD_NORMAL("Update ChangeNotifySeq, iUpdateSequence=[%lld],sUpdateTime=[%s],sTableName=[%s]", iUpdateSequence,sUpdateTime,m_tCurTable->sTableName);
        try
        {
        	m_pOraQry->Close();
    		m_pOraQry->SetSQL(m_tCurQuery->m_sUpdateNotifySeqSQL);
            m_pOraQry->SetParameter("update_time",sUpdateTime);
            m_pOraQry->SetParameter("TABLE_SEQUENCE",iUpdateSequence);
            m_pOraQry->Execute();
            m_pOraQry->Commit();
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        catch(TMdbException &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  Start()
    * 函数描述	:  同步Oracle数据
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回负数
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbChangeNotify::Start()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
    	int iCnt = 0;
        while(true)
        {
            if(m_tProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("Stop.");
                return 0;
            }
            //应用进程心跳时间
            m_tProcCtrl.UpdateProcHeart(0);

            if(0 != ConnectOracle()){TMdbDateTime::Sleep(1);continue;}
			m_bNeedFlush = false;
       		m_iTotalFlushCount = 0;
            TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
            for(;itor != m_pShmDSN->m_TableList.end();++itor)
            {
                TMdbTable* pTable =&(*itor);
                iRet = FlushTable(pTable);
            }
            if(m_bNeedFlush == true)
	        {
				iCnt = 0;
				if(m_iTotalFlushCount < 100)
				{
				  TMdbDateTime::Sleep(1);
				}
	        }
	        else
	        {
	            iCnt++;
	            if(iCnt > 3)
	            {
	                TADD_NORMAL("DSN[%s] does not have new data needs to be refreshed.",m_pDsn->sName);
	                iCnt--;
	                TMdbDateTime::Sleep(m_iTimeInterval);
	             }
	        }
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  FlushTable()
    * 函数描述	:  将某张表的Oracle变更数据刷新到QMDB中
    * 输入		:  表信息
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回负数
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbChangeNotify::FlushTable(TMdbTable* pTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        MDB_INT64 iStartSeq = -1;
        MDB_INT64 iUpdateSeq = -1;
        MDB_INT64 iEndSeq = -1;
		MDB_INT64 iTempStartSeq = -1;
        MDB_INT64 iTempEndSeq=-1;
        char sUpdateTime[MAX_TIME_LEN];
        memset(sUpdateTime,0,MAX_TIME_LEN);
        
        //ClearFlushList();
        if(false == FiltTable(pTable)){return iRet;}

        
        m_tCurTable = pTable;
        
        CHECK_RET(InitTable(),"Init TableQuery Faild,TableName=[%s]",m_tCurTable->sTableName);
        
        m_tCurQuery = GetTabQuery(m_tCurTable->sTableName);//m_tQueryList[m_tCurTable->iTableID];
        
        CHECK_RET(GetStartEndSeq(iStartSeq, iEndSeq),"GetStartSeq Faild,TableName=[%s]",m_tCurTable->sTableName);
        if(iEndSeq == -1) {return iRet;}
		iTempStartSeq = iStartSeq;
		iTempEndSeq = iStartSeq;
        while(1)
    	{
    		if(iTempEndSeq > iEndSeq) {break;}
			iTempStartSeq = iTempEndSeq;
			iTempEndSeq += 50000;
		    if(iTempEndSeq > iEndSeq && iEndSeq != -1)
		    {
		        iTempEndSeq = iEndSeq;
		    }
			CHECK_RET(InitFlushList(iTempStartSeq, iTempEndSeq), "InitFlushList failed");
		    FlushDRecord(iTempStartSeq,iTempEndSeq,iUpdateSeq,sUpdateTime);
		    FlushRecord(iTempStartSeq,iTempEndSeq,iUpdateSeq,sUpdateTime);
	        if(iUpdateSeq!=-1)
	        {
	            CHECK_RET(UpdateNotifySeqTable(iUpdateSeq,sUpdateTime),"UpdateNotifySeqTable Faild,TableName=[%s]",m_tCurTable->sTableName);
	            InsertDelayList(iTempStartSeq,iTempEndSeq);
	        }
			iTempEndSeq++;
		    /*else
		    {
		        //没有记录有两种情况，一种是确实没有记录，还有一种是间隔超过1万条
		        if(iEndSeq != -1 && iEndSeq != iStartSeq)
		        {
		            //这种情况是有记录但是最小的一条记录大于iStartSeq+MAX_REP_PREFETCH_ROWS
		            TADD_WARNING("deal with all record iStartSeq=[%lld],iEndSeq=[%lld],table_name=[%s]",iStartSeq, iEndSeq,m_tCurTable->sTableName);
		            iTempEndSeq = iEndSeq;
		            CHECK_RET(InitFlushList(iStartSeq, iTempEndSeq), "InitFlushList failed");
		            FlushDRecord(iStartSeq,iTempEndSeq,iUpdateSeq,sUpdateTime);
		            FlushRecord(iStartSeq,iTempEndSeq,iUpdateSeq,sUpdateTime);
		            if(iUpdateSeq!=-1)
		            {
		                CHECK_RET(UpdateNotifySeqTable(iUpdateSeq,sUpdateTime),"UpdateNotifySeqTable Faild,TableName=[%s]",m_tCurTable->sTableName);
		                InsertDelayList(iStartSeq,iTempEndSeq);
		            }
		        }
		    }*/
    	}
        //处理漏刷链表(影子表模式下不会存在漏刷节点)
        CHECK_RET(DealDelayData(),"DealDelayData Faild,TableName=[%s]",m_tCurTable->sTableName);

        // 一张表刷新完成，将剩余的刷新记录信息打印下
        PrintFlushLog();

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbChangeNotify::FlushRecord(MDB_INT64 &iStartSeq,MDB_INT64 &iEndSeq,MDB_INT64 &iUpdateSequence,char *sUpdateTime)
    {
    	TADD_FUNC("Start.");	
    	int iRet = 0;
    	MDB_INT64 iCurSequence = -1;
    	MDB_INT64 iLastSequence = iStartSeq;
    	char ActionType;
        try
        {
        	m_pOraQry->Close();
			m_pOraQry->Commit();
        	m_pOraQry->Close();
    		m_pOraQry->SetSQL(m_tCurQuery->m_sOraSelectSQL);
        	//设置参数
    	    if(m_tCurTable->iParameterCount != 0 && m_tCurTable->m_sFlushSQL[0] != 0)
    	    {
    	        for(int i = 0; i<m_tCurTable->iParameterCount; i++)
    	        {
    	            if(m_tCurTable->tParameter[i].iDataType == DT_Int && m_tCurTable->tParameter[i].iParameterType  == 1)
    	            {
    	                m_pOraQry->SetParameter(m_tCurTable->tParameter[i].sName,TMdbNtcStrFunc::StrToInt(m_tCurTable->tParameter[i].sValue));
    	            }
    	            else if(m_tCurTable->tParameter[i].iParameterType == 1)
    	            {
    	                m_pOraQry->SetParameter(m_tCurTable->tParameter[i].sName,m_tCurTable->tParameter[i].sValue);
    	            }
    	        }
    	    }
    	    m_pOraQry->SetParameter("P_MIN_TABLE_SEQUANCE",iStartSeq);
    	    m_pOraQry->SetParameter("P_MAX_TABLE_SEQUANCE",iEndSeq);
            TMdbDateTime::GetCurrentTimeStr(m_sOpenStartTime);
    		m_pOraQry->Open(MAX_REP_PREFETCH_ROWS);
            TMdbDateTime::GetCurrentTimeStr(m_sOpenEndTime);
            if(TMdbDateTime::GetDiffSeconds(m_sOpenEndTime, m_sOpenStartTime) > MAX_OPEN_CURSOR_SEC)
            {
                TADD_WARNING("Open cursor more than %d seconds!TABLE[%s],iStartSeq=%lld, iEndSeq=%lld "
                                              ,MAX_OPEN_CURSOR_SEC,m_tCurTable->sTableName, iStartSeq, iEndSeq);
            }
            while(m_pOraQry->Next())
            {
                m_sRecdLogStr.clear();
                iCurSequence = m_pOraQry->Field("TABLE_SEQUENCE").AsInteger();
    			ActionType = m_pOraQry->Field("ACTION_TYPE").AsString()[0];
                if( iCurSequence >=  iLastSequence)
                {
                    iUpdateSequence = iCurSequence;
    				iLastSequence = iCurSequence;
    				iEndSeq = iCurSequence;
                    strcpy(sUpdateTime,m_pOraQry->Field("UPDATE_TIME").AsDateTimeString());
                }
                TADD_DETAIL("iCurSequence=[%lld],ActionType=[%c],sTableName=[%s]", iCurSequence,ActionType,m_tCurTable->sTableName);		
                LogRecdNotifyInfo(iStartSeq, iEndSeq,iCurSequence, ActionType, m_tCurTable->sTableName, m_sRecdLogStr);
                
    			if(AddToFlushList(iStartSeq, iCurSequence) <0 )
    		    {
    		        TADD_ERROR(ERROR_UNKNOWN,"iCurSequence=[%lld] AddToFlushList failed", iCurSequence);
    		    }

    			if(ActionType != 'D')
    			{
    	            iRet = QueryData();
    	            if(iRet == 0)
    	            {
    	                iRet = InsertData();
    	            }
    	            else if(iRet == 1)
    	            {
    	                iRet = UpdateData();
    	            }

                    LogFlushInfo(m_sRecdLogStr);
    			}
    			else
    			{
    				continue;
    			}
            	m_bNeedFlush = true;
            }
    	}
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        catch(TMdbException &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
    	TADD_FUNC("Finish.");

    	return iRet;
    }

    int TMdbChangeNotify::FlushDRecord(MDB_INT64 &iStartSeq,MDB_INT64 &iEndSeq,MDB_INT64 &iUpdateSequence,char *sUpdateTime)
    {
    	TADD_FUNC("Start.");	
    	int iRet = 0;
    	MDB_INT64 iCurSequence = -1;
    	MDB_INT64 iLastSequence = iStartSeq;
    	char ActionType;
        try
        {
        	m_pOraQry->Close();
			m_pOraQry->Commit();
        	m_pOraQry->Close();
    		m_pOraQry->SetSQL(m_tCurQuery->m_sOraDeleteSQL);
    	    m_pOraQry->SetParameter("P_MIN_TABLE_SEQUANCE",iStartSeq);
    	    m_pOraQry->SetParameter("P_MAX_TABLE_SEQUANCE",iEndSeq);
            TMdbDateTime::GetCurrentTimeStr(m_sOpenStartTime);
    		m_pOraQry->Open(MAX_REP_PREFETCH_ROWS);
            TMdbDateTime::GetCurrentTimeStr(m_sOpenEndTime);
            if(TMdbDateTime::GetDiffSeconds(m_sOpenEndTime, m_sOpenStartTime) > MAX_OPEN_CURSOR_SEC)
            {
                TADD_WARNING("Open cursor more than %d seconds!TABLE[%s],iStartSeq=%lld, iEndSeq=%lld "
                                              ,MAX_OPEN_CURSOR_SEC, m_tCurTable->sTableName, iStartSeq, iEndSeq);
            }
            while(m_pOraQry->Next())
            {
                m_sRecdLogStr.clear();
                iCurSequence = m_pOraQry->Field("TABLE_SEQUENCE").AsInteger();
    			ActionType = m_pOraQry->Field("ACTION_TYPE").AsString()[0];
                if( iCurSequence >=  iLastSequence)
                {
                    iUpdateSequence = iCurSequence;
    				iLastSequence = iCurSequence;
    				iEndSeq = iCurSequence;
                    strcpy(sUpdateTime,m_pOraQry->Field("UPDATE_TIME").AsDateTimeString());
                }
                TADD_DETAIL("iCurSequence=[%lld],ActionType=[%c],sTableName=[%s]", iCurSequence,ActionType,m_tCurTable->sTableName);		
                LogRecdNotifyInfo(iStartSeq, iEndSeq,iCurSequence,ActionType,m_tCurTable->sTableName, m_sRecdLogStr);
                if(AddToFlushList(iStartSeq, iCurSequence) <0 )
    		    {
    		        TADD_ERROR(ERROR_UNKNOWN,"iCurSequence=[%lld] AddToFlushList failed", iCurSequence);
    		    }
    			iRet = DeleteData();
                LogFlushInfo(m_sRecdLogStr);
            	m_bNeedFlush = true;
            }
    	}
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        catch(TMdbException &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
    	TADD_FUNC("Finish.");

    	return iRet;
    }
    /******************************************************************************
    * 函数名称	:  FiltTable()
    * 函数描述	:  过滤不需要刷新的表
    * 输入		:  表信息
    * 输出		:  无
    * 返回值	:  成功返回Ture，否则返回False
    * 作者		:  dong.chun
    *******************************************************************************/
    bool TMdbChangeNotify::FiltTable(TMdbTable* pTable)
    {
    	TADD_FUNC("Start.");
    	if(NULL == pTable){return false;}
    	if(pTable->bIsSysTab){return false;}
        if(pTable->iRepAttr == REP_FROM_DB)
        {
            if(m_pConfig->IsNotLoadFromDB(pTable->sTableName))
            {//配置了不加载
                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            return false;
        }
    	TADD_FUNC("Finish.");
    	return true;
    }
    /******************************************************************************
    * 函数名称	:  ClearAllTableQuery()
    * 函数描述	:  请空所有表的query对象
    * 输入		:  表信息
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回负数
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbChangeNotify::ClearAllTableQuery()
    {
    	TADD_FUNC("Start.");

        SAFE_DELETE(m_pOraQry);
        /*
        for(int i =0; i<MAX_TABLE_COUNTS; i++)
        {
            if(m_tQueryList[i] == NULL){continue;}
    		SAFE_DELETE(m_tQueryList[i]);
        }
        */
    	TADD_FUNC("Finish.");
    	return 0;
    }
    /******************************************************************************
    * 函数名称	:  ClearAllTableQuery()
    * 函数描述	:  初始化表的所有query对象
    * 输入		:  表信息
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回负数
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbChangeNotify::InitTable()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(GetTabQuery(m_tCurTable->sTableName)!= NULL){return iRet;}
        int i =0;
        for(i =0; i < MAX_TABLE_COUNTS;i++)
        {
            if(m_tQueryList[i] == NULL)
            {
                break;
            }
        }
        m_tQueryList[i] = new(std::nothrow) CNQueryList();
        CHECK_OBJ(m_tQueryList[i]);
         iRet= m_tQueryList[i]->Init(m_mdbLink,m_pDBLink,m_tCurTable, m_pConfig,m_pDsn->sName,m_sChangeNotifyName,m_sChangeNotifySeqName);

        if(iRet != 0){SAFE_DELETE(m_tQueryList[i]);}
        TADD_FUNC("Finish.");
        return iRet;
    }
    //删除数据, 不进行主备机同步、不进行Oracle同步
    int TMdbChangeNotify::DeleteData()
    {
        TADD_FUNC("Start.");
        char sValue[1024];
        memset(sValue,0,sizeof(sValue));
        TMdbQuery *pCurQuery = NULL;
        
        //执行删除操作
        try
        {
            TADD_DETAIL("pTable_name=[%s]",m_tCurTable->sTableName);

            int iFlag = m_tCurQuery->GetDeleteSQL(m_pOraQry);
            if(iFlag > 0)
            {
                m_tCurQuery->m_pQueryD->SetSQL(m_tCurQuery->m_sSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
            }
            
            pCurQuery= m_tCurQuery->m_pQueryD;

            for(int i=0; i<m_tCurTable->m_tPriKey.iColumnCounts; ++i)
            {
                if(m_pOraQry->Field(i).isNULL())
                {
                    continue;
                }

                int iColumnNo = m_tCurTable->m_tPriKey.iColumnNo[i];

                if(m_tCurTable->tColumn[iColumnNo].iDataType == DT_Int)
                {
                    TADD_DETAIL("column_name=[%s],column_value=[%lld]",m_tCurTable->tColumn[iColumnNo].sName,atoll(m_pOraQry->Field(i).AsString()));
                    pCurQuery->SetParameter(m_tCurTable->tColumn[iColumnNo].sName, atoll(m_pOraQry->Field(i).AsString()));
                    LogRecdPkInfo( m_sRecdLogStr, m_tCurTable->tColumn[iColumnNo].sName, m_pOraQry->Field(iColumnNo).AsString());
                }
                else if(m_tCurTable->tColumn[iColumnNo].iDataType != DT_Blob)
                {
                    TADD_DETAIL("column_name=[%s],column_value=[%s]",m_tCurTable->tColumn[iColumnNo].sName,m_pOraQry->Field(i).AsString());
                    memset(sValue, 0, sizeof(sValue));
                    SAFESTRCPY(sValue,sizeof(sValue),m_pOraQry->Field(i).AsString());
                    TMdbNtcStrFunc::Trim(sValue);
                    pCurQuery->SetParameter(m_tCurTable->tColumn[iColumnNo].sName,sValue);
                    LogRecdPkInfo( m_sRecdLogStr, m_tCurTable->tColumn[iColumnNo].sName, sValue);
                }
            }
            
            pCurQuery->Execute();
            pCurQuery->Commit();
            m_tCurQuery->m_iFlushCount++;
            m_iTotalFlushCount++;
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        catch(TMdbException &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }

        TADD_FUNC("Finish.");

        return 0;
    }



    //插入数据
    int TMdbChangeNotify::InsertData()
    {
        TADD_FUNC("Start.");
    	int iRet = 0;
        char sValue[1024];
        memset(sValue,0,sizeof(sValue));
        try
        {
            TADD_DETAIL("pTable_name=[%s]",m_tCurTable->sTableName);
            for(int i=0; i<m_tCurTable->iColumnCounts; ++i)
            {
                if(m_pOraQry->Field(i).isNULL() && false == m_tCurTable->tColumn[i].IsStrDataType())
                {
                    m_tCurQuery->m_pQueryI->SetParameterNULL(m_tCurTable->tColumn[i].sName);
                }
                else if(m_tCurTable->tColumn[i].iDataType == DT_Int)
                {
                     m_tCurQuery->m_pQueryI->SetParameter(m_tCurTable->tColumn[i].sName,m_pOraQry->Field(i).AsInteger());
                    TADD_DETAIL("column_name=[%s],column_value=[%ld]",m_tCurTable->tColumn[i].sName,m_pOraQry->Field(i).AsInteger());
                }
                else if(m_tCurTable->tColumn[i].iDataType != DT_Blob)
                {
                    memset(sValue, 0, sizeof(sValue));
                    SAFESTRCPY(sValue,sizeof(sValue),m_pOraQry->Field(i).AsString());
                    TMdbNtcStrFunc::Trim(sValue);
                     m_tCurQuery->m_pQueryI->SetParameter(m_tCurTable->tColumn[i].sName, sValue);
                    TADD_DETAIL("column_name=[%s],column_value=[%s]",m_tCurTable->tColumn[i].sName,m_pOraQry->Field(i).AsString());
                }
            }

    		m_tCurQuery->m_pQueryI->Execute();
    		m_tCurQuery->m_pQueryI->Commit();
            m_tCurQuery->m_iFlushCount++;
            m_iTotalFlushCount++;
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        catch(TMdbException &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }

        TADD_FUNC("Finish.");
        return iRet;
    }


    //更新数据
    int TMdbChangeNotify::UpdateData()
    {
        TADD_FUNC("Start.");
        char sValue[1024];
        memset(sValue,0,sizeof(sValue));
        int iRet = 0;
        try
        {
            TADD_DETAIL("pTable_name=[%s]",m_tCurTable->sTableName);
            TADD_DETAIL("iColumnCounts = [%d].", m_tCurTable->iColumnCounts);
            // 所有列均为主键列，不支持update
            if(m_tCurTable->iColumnCounts == m_tCurTable->m_tPriKey.iColumnCounts)
            {
                TADD_WARNING("Table[%s]:All Columns are primary key, update not support", m_tCurTable->sTableName);
                return 0;
            }
            
            for(int i=0; i<m_tCurTable->iColumnCounts; ++i)
            {
                TADD_DETAIL("Column[%d]-Name=[%s].", i, m_tCurTable->tColumn[i].sName);

    			if(isKey(i) == true)
    			{
    				continue;
    			}
    			
                if(m_pOraQry->Field(i).isNULL() && false == m_tCurTable->tColumn[i].IsStrDataType())
                {
                     m_tCurQuery->m_pQueryU->SetParameterNULL(m_tCurTable->tColumn[i].sName);
                }
                else if(m_tCurTable->tColumn[i].iDataType == DT_Int)
                {
                    TADD_DETAIL("Int-Value=[%ld].", m_pOraQry->Field(i).AsInteger());
                    m_tCurQuery->m_pQueryU->SetParameter(m_tCurTable->tColumn[i].sName,m_pOraQry->Field(i).AsInteger());
                }
                else if(m_tCurTable->tColumn[i].iDataType != DT_Blob)
                {
                    TADD_DETAIL("String-Value=[%s].", sValue);
                    memset(sValue, 0, sizeof(sValue));
                    SAFESTRCPY(sValue,sizeof(sValue),m_pOraQry->Field(i).AsString());
                    TMdbNtcStrFunc::Trim(sValue);
                    m_tCurQuery->m_pQueryU->SetParameter(m_tCurTable->tColumn[i].sName, sValue);
                }
            }
            //最后拼写主健字段
            TADD_DETAIL("m_tPriKey.iColumnCounts = [%d].", m_tCurTable->m_tPriKey.iColumnCounts);
            for(int i=0; i<m_tCurTable->m_tPriKey.iColumnCounts; ++i)
            {
                int iColumnNo = m_tCurTable->m_tPriKey.iColumnNo[i];

                char sTemp[128];
                memset(sTemp, 0, sizeof(sTemp));
                sprintf(sTemp, "%s%d", m_tCurTable->tColumn[iColumnNo].sName, iColumnNo);
                if(m_pOraQry->Field(iColumnNo).isNULL() && false == m_tCurTable->tColumn[iColumnNo].IsStrDataType())
                {
                    m_tCurQuery->m_pQueryU->SetParameterNULL(sTemp);
                }
                else if(m_tCurTable->tColumn[iColumnNo].iDataType == DT_Int)
                {
                    TADD_DETAIL("column_name=[%s],column_value=[%ld]", sTemp, m_pOraQry->Field(iColumnNo).AsInteger());
                    m_tCurQuery->m_pQueryU->SetParameter(sTemp, m_pOraQry->Field(iColumnNo).AsInteger());
                }
                else if(m_tCurTable->tColumn[i].iDataType == DT_Blob)
                {
                    TADD_ERROR(ERR_SQL_INVALID,"not support blob flush");
                    return ERR_SQL_INVALID;
                }
                else if(m_tCurTable->tColumn[i].iDataType != DT_Blob)
                {
                    memset(sValue, 0, sizeof(sValue));
                    SAFESTRCPY(sValue,sizeof(sValue),m_pOraQry->Field(iColumnNo).AsString());
                    TMdbNtcStrFunc::Trim(sValue);
                    TADD_DETAIL("column_name=[%s],column_value=[%ld].", sTemp, sValue);
                    m_tCurQuery->m_pQueryU->SetParameter(sTemp, sValue);
                }
            }
            m_tCurQuery->m_pQueryU->Execute();
            m_tCurQuery->m_pQueryU->Commit();
            m_tCurQuery->m_iFlushCount++;
            m_iTotalFlushCount++;
    	}
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        catch(TMdbException &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }



    //插入数据, 不进行主备机同步、不进行Oracle同步
    int TMdbChangeNotify::QueryData()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            //设置主健字段
            for(int i=0; i<m_tCurTable->m_tPriKey.iColumnCounts; ++i)
            {
                int iColumnNo = m_tCurTable->m_tPriKey.iColumnNo[i];
                if(m_pOraQry->Field(iColumnNo).isNULL() && false == m_tCurTable->tColumn[iColumnNo].IsStrDataType())
                {
                    m_tCurQuery->m_pQueryQ->SetParameterNULL(m_tCurTable->tColumn[iColumnNo].sName);
                    LogRecdPkInfo( m_sRecdLogStr, m_tCurTable->tColumn[iColumnNo].sName, "NULL");
                }
                else if(m_tCurTable->tColumn[iColumnNo].iDataType == DT_Int)
                {
                    m_tCurQuery->m_pQueryQ->SetParameter(m_tCurTable->tColumn[iColumnNo].sName, m_pOraQry->Field(iColumnNo).AsInteger());
                    LogRecdPkInfo( m_sRecdLogStr, m_tCurTable->tColumn[iColumnNo].sName, m_pOraQry->Field(iColumnNo).AsString());
                }
                else if(m_tCurTable->tColumn[iColumnNo].iDataType != DT_Blob)
                {
                    m_tCurQuery->m_pQueryQ->SetParameter(m_tCurTable->tColumn[iColumnNo].sName, m_pOraQry->Field(iColumnNo).AsString());
                    LogRecdPkInfo( m_sRecdLogStr, m_tCurTable->tColumn[iColumnNo].sName, m_pOraQry->Field(iColumnNo).AsString());
                }
            }
            m_tCurQuery->m_pQueryQ->Open();
            if(m_tCurQuery->m_pQueryQ->Next())
            {
                TADD_DETAIL("Find-Data to Query.");
                return 1;
            }
            else
            {
                TADD_DETAIL("No-Data to Query.");
                return 0;
            }
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        catch(TMdbException &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
    	return iRet;
    }


    //把漏掉的数据插入"漏刷链表"
    void TMdbChangeNotify::InsertDelayList(MDB_INT64 iStart, MDB_INT64 iEnd)
    {
        TADD_FUNC("Start.");
        char cFlag ='N';
        MDB_INT64 lIndex = 0;
        for(MDB_INT64 i = iStart;i<=iEnd;i++)
        {
            lIndex = i - iStart;
            if(m_bAlloc)
            {
                cFlag = m_pFlushList[lIndex];
            }
            else
            {
                cFlag = m_cFlushList[lIndex];
            }
            TADD_DETAIL("cFlag=%c",cFlag);
            if('Y' != cFlag )
            {
                LSNODE vLsNode;
                vLsNode.iSeq = i;
                SAFESTRCPY(vLsNode.sTabName, sizeof(vLsNode.sTabName), m_tCurTable->sTableName);
                TMdbDateTime::GetCurrentTimeStr(vLsNode.sTime);
                TADD_NORMAL("InsertDelayList,iSeq=[%lld], Table=[%s],sTime=[%s].", vLsNode.iSeq, vLsNode.sTabName,vLsNode.sTime);
                m_vLsList.push_back(vLsNode);
            }
        }

        TADD_FUNC("Finish.");
        return;
    }


    //处理"漏刷链表"中的数据
    int TMdbChangeNotify::DealDelayData()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        MDB_INT64 iUpdateSeq = -1;
        char sUpdateTime[MAX_TIME_LEN];
        memset(sUpdateTime,0,MAX_TIME_LEN);
        char sCurTime[32];
        memset(sCurTime, 0, sizeof(sCurTime));
        std::vector<LSNODE >::iterator itor = m_vLsList.begin();
        for(; itor != m_vLsList.end();)
        {
            LSNODE vLsNode = (*itor);
            if(0 != TMdbNtcStrFunc::StrNoCaseCmp(vLsNode.sTabName, m_tCurTable->sTableName)){++itor;continue;}//只处理与当前表关联的数据

            iUpdateSeq = -1;
            iRet = FlushRecord(vLsNode.iSeq,vLsNode.iSeq,iUpdateSeq,sUpdateTime);
            if(iUpdateSeq == -1)
            {
                iRet = FlushDRecord(vLsNode.iSeq,vLsNode.iSeq,iUpdateSeq,sUpdateTime);
            }

            if(iUpdateSeq != -1)
            {
                itor = m_vLsList.erase(itor); 
                continue;
            }//如果查询到，则删除该节点

            memset(sCurTime, 0, sizeof(sCurTime));
            TMdbDateTime::GetCurrentTimeStr(sCurTime);
            if(TMdbDateTime::GetDiffSeconds(sCurTime, vLsNode.sTime) > m_iDelayTime) //3
            {
                TADD_NORMAL("Seq TimeOut,iSeqID=[%lld], sCurTime = [%s],sTime = [%s],iTableName=[%s].",vLsNode.iSeq,sCurTime,vLsNode.sTime,m_tCurTable->sTableName);
                itor = m_vLsList.erase(itor);

                if(iRet < 0)
                {
                    return iRet;
                }
                continue;
            }

            if(iRet < 0)
            {
                return iRet;
            }
            ++itor;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbChangeNotify::ConnectOracle()
    {
    	TADD_FUNC("Start.");
    	int iRet = 0;
    	//如果链接不为空，则判断链接是否正常，如果正常则直接返回，如果不正常，则清空表的query，并清空链接，重新创建链接
    	if(m_pDBLink != NULL)
    	{
    		if(m_pDBLink->IsConnect() == true){return iRet;}
    		CHECK_RET(ClearAllTableQuery(),"ClearAllTableQuery Faild");
    		m_pDBLink->Disconnect();
    		SAFE_DELETE(m_pDBLink);
    	}
        try
        {
            m_pDBLink = TMDBDBFactory::CeatDB();
    		CHECK_OBJ(m_pDBLink);
            m_pDBLink->SetLogin(m_pConfig->GetDSN()->sOracleUID, m_pConfig->GetDSN()->sOraclePWD, m_pConfig->GetDSN()->sOracleID);
    		if(m_pDBLink->Connect(true) == false)
    		{
    			TADD_ERROR(ERR_DB_NOT_CONNECTED,"Connect Oracle Faild,user=[%s],pwd=[%s],dsn=[%s].",m_pConfig->GetDSN()->sOracleUID,m_pConfig->GetDSN()->sOraclePWD,m_pConfig->GetDSN()->sOracleID);
    			return ERR_DB_NOT_CONNECTED;
    		}

            CHECK_RET(InitOraQry(),"init oracle query failed");
    		CHECK_RET(SetSqlName(),"SetSqlName Faild");
    		
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_DB_NOT_CONNECTED;
        }
    	TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbChangeNotify::GetStartEndSeq(MDB_INT64 &iStartSeq, MDB_INT64 &iEndSeq)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        bool bExist = false;
        try
        {
            m_pOraQry->Close();
			m_pOraQry->Commit();
        	m_pOraQry->Close();
            m_pOraQry->SetSQL(m_tCurQuery->m_sGetSeqFromNotifySeqSQL);
            //获取change_notify_seq表的seq值
            m_pOraQry->Open();
            while(m_pOraQry->Next())
            {
                bExist = true;
                iStartSeq = m_pOraQry->Field("TABLE_SEQUENCE").AsInteger();
            }
            
            m_pOraQry->Close();
			m_pOraQry->Commit();
        	m_pOraQry->Close();
            m_pOraQry->SetSQL(m_tCurQuery->m_sGetSeqFromNotifySQL);
            //对于change_notify_seq表中有记录并且table_sequence只为-1的情况并且
            //change_notify表中也有刷新记录的情况下，需要重新获取起始序列
            m_pOraQry->Open();
            while(m_pOraQry->Next())
            {
                iEndSeq = m_pOraQry->Field("MAX_TABLE_SEQUENCE").AsInteger();
				if( iEndSeq != -1)
				{
					if(bExist && iStartSeq <= 0)
	                {
	                    iStartSeq = m_pOraQry->Field("MIN_TABLE_SEQUENCE").AsInteger();
	                }
	                else
	                {
	                    iStartSeq++;
	                }
				}
            }

            if(iEndSeq != -1 && iEndSeq <= iStartSeq)
            {
                TADD_DETAIL("MAX_TABLE_SEQUENCE(%lld) <= MIN_TABLE_SEQUENCE(%lld), set MAX_TABLE_SEQUENCE = MIN_TABLE_SEQUENCE", iEndSeq, iStartSeq);
                iEndSeq = iStartSeq;
            }

            //插入初始化纪录
            if (false == bExist)
            {
                iStartSeq = GetSmartStartSeq(iStartSeq);
                m_pOraQry->Close();
                m_pOraQry->SetSQL(m_tCurQuery->m_sInsertNotifySeqSQL);
                m_pOraQry->SetParameter("TABLE_SEQUENCE", iStartSeq);
                m_pOraQry->Execute();
                m_pOraQry->Commit();
            }
            
            TADD_FLOW("Get StartSeq=[%lld], EndSeq=[%lld],TableName=[%s].",\
                iStartSeq,iEndSeq,m_tCurTable->sTableName);
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    CNQueryList* TMdbChangeNotify::GetTabQuery(const char* psTabName)
    {
        for(int i=0; i < MAX_TABLE_COUNTS; i++)
        {
            if(m_tQueryList[i] == NULL)
            {
                continue;
            }

            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTabName, m_tQueryList[i]->m_pTable->sTableName))
            {
                return m_tQueryList[i];
            }
        }
        return NULL;
    }

    MDB_INT64 TMdbChangeNotify::GetSmartStartSeq(MDB_INT64 lStartSeq)
    {
        TADD_FUNC("Start.");
        MDB_INT64 iRet = 0;
        MDB_INT64 iMinSeq = 0;

        TADD_DETAIL("input start seq =%lld",lStartSeq);

        try
        {
            char sSql[MAX_SQL_LEN] = {0};

            sprintf(sSql, "select nvl(min(TABLE_SEQUENCE),-1) MIN_TABLE_SEQUENCE from %s where table_name = '%s'",
                m_sChangeNotifyName,m_tCurTable->sTableName);

            TADD_DETAIL("SQL=[%s]", sSql);
            
            m_pOraQry->Close();
			m_pOraQry->Commit();
			m_pOraQry->Close();
            m_pOraQry->SetSQL(sSql);
            m_pOraQry->Open();
            while(m_pOraQry->Next())
            {
                iRet = m_pOraQry->Field("MIN_TABLE_SEQUENCE").AsInteger();
                iMinSeq = iRet;
            }

            TADD_DETAIL("min seq =%lld",iRet);

            // 若%DSN%_MDB_CHANGE_NOTIF表中没有该表的刷新记录时，使用原始的start seq(即1)
            if(iRet < lStartSeq )
            {
                iRet = lStartSeq;
            }

            TADD_NORMAL("Reset start seq =%lld.(old start seq =%lld,min_table_sequence=%lld).",iRet,lStartSeq, iMinSeq);
            
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  InitOraQry()
    * 函数描述	:  初始化 %dsn%_mdb_change_notif表查询句柄
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回负数
    * 作者		:  li.ming
    *******************************************************************************/
    int TMdbChangeNotify::InitOraQry()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            if(NULL != m_pOraQry)
            {
                SAFE_DELETE(m_pOraQry);
            }
            
            m_pOraQry = m_pDBLink->CreateDBQuery();
            CHECK_OBJ(m_pOraQry);
            
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Init failed : ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
            TADD_FUNC("Finish.");
            return -1;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Init failed : Unknown error!\n");
            TADD_FUNC("Finish.");
            return -1;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  NeedToFlush()
    * 函数描述	:  查询 %dsn%_mdb_change_notif表,判断是否有需要待刷新的数据
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  需要遍历刷新返回true，否则返回false
    * 作者		:  li.ming
    *******************************************************************************/
    bool TMdbChangeNotify::NeedToFlush()
    {
        TADD_FUNC("Start.");
        bool bRet = true;
        int iDealyTime = 0;
        int iCnt = 0;
        if(m_sQrySql[0] == 0)
        {
            if(true == m_pConfig->GetDSN()->m_bShadow)
            {
                #ifdef DB_ORACLE
                    sprintf(m_sQrySql, " select count(1) COUNTS from %s A, "
                    " (select nvl(max(update_time),TO_DATE('19700102000000','YYYYMMDDHH24MISS')) max_update_time from %s) B "
                    " where B.max_update_time >= (sysdate - :P_DELAY_TIME/(24*3600)) "
                    , m_sShadowName,m_sShadowName);
                #elif DB_MYSQL
                    sprintf(m_sQrySql, " select count(1) COUNTS from %s A, "
                    " (select nvl(max(update_time),TO_DATE('19700102000000','YYYYMMDDHH24MISS')) max_update_time from %s) B "
                    " where B.max_update_time >= (sysdate - interval :P_DELAY_TIME second) "
                    , m_sShadowName,m_sShadowName);
                #else
                    TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
                    return false;
                #endif             
            }
            else
            {
                #ifdef DB_ORACLE
                    sprintf(m_sQrySql, " select count(1) COUNTS from %s A, "
                        " (select nvl(max(update_time),TO_DATE('19700102000000','YYYYMMDDHH24MISS')) max_update_time from %s) B "
                        " where B.max_update_time >= (sysdate - :P_DELAY_TIME/(24*3600)) "
                        , m_sChangeNotifyName,m_sChangeNotifyName);
                #elif DB_MYSQL
                    sprintf(m_sQrySql, " select count(1) COUNTS from %s A, "
                        " (select nvl(max(update_time),TO_DATE('19700102000000','YYYYMMDDHH24MISS')) max_update_time from %s) B "
                        " where B.max_update_time >= (sysdate - interval :P_DELAY_TIME second) "
                        , m_sChangeNotifyName,m_sChangeNotifyName);
                #else
                    TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
                    return false;
                #endif 
            
            }            

            TADD_DETAIL("SQL=[%s]", m_sQrySql);
        }

        
        try
        {
            iDealyTime = 2* (m_iTimeInterval + m_iDelayTime);
            m_pOraQry->Close();
            m_pOraQry->SetSQL(m_sQrySql);
            m_pOraQry->SetParameter("P_DELAY_TIME", iDealyTime);
            m_pOraQry->Open();
            m_pOraQry->Next();
            iCnt = m_pOraQry->Field("COUNTS").AsInteger();
            TADD_DETAIL("counts = %d", iCnt);
            if(iCnt > 0)
            {
                bRet = true;
            }
            else
            {
                bRet = false;
                TADD_NORMAL("DSN[%s] does not have new data needs to be refreshed.",m_pDsn->sName);
            }
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Init failed : ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
            TADD_FUNC("Finish.");
            return bRet;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Init failed : Unknown error!\n");
            TADD_FUNC("Finish.");
            return bRet;
        }
        TADD_FUNC("Finish.");
        return bRet;
    }


    int TMdbChangeNotify::InitFlushList(MDB_INT64 lStartSeq, MDB_INT64 lEndSeq)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_bAlloc = false;
        m_lStartSeq = lStartSeq;
        m_lEndSeq = lEndSeq;
        m_lFlushCnt = lEndSeq - lStartSeq+1;
        if(m_lFlushCnt < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"StartSeq=%lld, EndSeq=%lld, StartSeq > EndSeq", lStartSeq, lEndSeq);
            return -1;
        }

        if(m_lFlushCnt < MAX_FLUSH_ARRAY_CNT && m_lFlushCnt >= 0)
        {
            memset(m_cFlushList, 0, MAX_FLUSH_ARRAY_CNT);
        }
        else if(m_lFlushCnt >= MAX_FLUSH_ARRAY_CNT)
        {
            SAFE_DELETE_ARRAY(m_pFlushList);
            m_pFlushList = new char[m_lFlushCnt];
            CHECK_OBJ(m_pFlushList);
            memset(m_pFlushList, 0, m_lFlushCnt);
            m_bAlloc = true;
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbChangeNotify::AddToFlushList(MDB_INT64 lStartSeq, MDB_INT64 lSeq)
    {
        TADD_FUNC("Start.");
        int iRet =0 ;
        MDB_INT64 lIndex = lSeq - lStartSeq;
        if(lIndex < 0 || lIndex > m_lFlushCnt)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Invalid seq[%lld], startseq=[%lld], endseq=[%lld]", lSeq, lStartSeq, m_lEndSeq);
            return -1;
        }
        
        if(m_bAlloc && NULL != m_pFlushList)
        {
            m_pFlushList[lIndex] = 'Y';
        }
        else
        {
            m_cFlushList[lIndex] = 'Y';
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    void TMdbChangeNotify::ClearFlushList()
    {
        TADD_FUNC("Start.");
        
        if(m_bAlloc)
        {
            SAFE_DELETE_ARRAY(m_pFlushList);
            m_bAlloc = false;
        }
        else
        {
            memset(m_cFlushList, 0, sizeof(m_cFlushList));
        }

        m_lFlushCnt = 0;
        m_lStartSeq = 0;
        m_lEndSeq = 0;
        
        TADD_FUNC("Finish.");
    }

    int TMdbChangeNotify::SetSqlName()
    {
        char sChangeNotifySeqSQL[MAX_SQL_LEN];
        char sChangeNotifySQL[MAX_SQL_LEN];
        memset(sChangeNotifySeqSQL,0,sizeof(sChangeNotifySeqSQL));
        memset(sChangeNotifySQL,0,sizeof(sChangeNotifySQL));
        sprintf(sChangeNotifySeqSQL, "select TABLE_SEQUENCE from %s_MDB_CHANGE_NOTIFY_SEQ",m_pDsn->sName);
        sprintf(sChangeNotifySQL, "select TABLE_SEQUENCE from %s_MDB_CHANGE_NOTIF",m_pDsn->sName);
        try
        {
            CHECK_OBJ(m_pOraQry);
            m_pOraQry->Close();
            m_pOraQry->SetSQL(sChangeNotifySeqSQL);
            m_pOraQry->Open();
            if(m_pOraQry->Next())
            {
            }
            snprintf(m_sChangeNotifySeqName,sizeof(m_sChangeNotifySeqName),"%s_MDB_CHANGE_NOTIFY_SEQ",m_pDsn->sName);
        }
        catch(TMDBDBExcpInterface& e)
        {
            snprintf(m_sChangeNotifySeqName,sizeof(m_sChangeNotifySeqName),"MDB_CHANGE_NOTIFY_SEQ");
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Init failed : Unknown error!\n");
            TADD_FUNC("Finish.");
            return -1;
        }

        try
        {
            m_pOraQry->Close();
            m_pOraQry->SetSQL(sChangeNotifySQL);
            m_pOraQry->Open();
            if(m_pOraQry->Next())
            {
            }
            snprintf(m_sChangeNotifyName,sizeof(m_sChangeNotifyName),"%s_MDB_CHANGE_NOTIF",m_pDsn->sName);
        }
        catch(TMDBDBExcpInterface& e)
        {
            snprintf(m_sChangeNotifyName,sizeof(m_sChangeNotifyName),"MDB_CHANGE_NOTIF");
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Init failed : Unknown error!\n");
            TADD_FUNC("Finish.");
            return -1;
        }

        snprintf(m_sShadowName, sizeof(m_sShadowName), "QMDB_SHADOW_%s",m_pDsn->sName);

        TADD_NORMAL("m_sChangeNotifyName=[%s],m_sChangeNotifySeqName=[%s], shadowname=[%s]\n"
                    ,m_sChangeNotifyName,m_sChangeNotifySeqName,m_sShadowName);
        return 0;
    }

    void TMdbChangeNotify::PrintFlushLog()
    {
        TADD_FUNC("Start.");

        if(m_sFlushLogStr.size() <= 0)
        {
            TADD_DETAIL("no flush log to print");
            return;
        }
        
        TADD_NORMAL("%s",  m_sFlushLogStr.c_str());

        m_sFlushLogStr.clear();
        
        TADD_FUNC("Finish.");
        return;
    }

    void TMdbChangeNotify::LogFlushInfo(std::string&  sLogInfo)
    {
        TADD_FUNC("Start.");
        
        m_sFlushLogStr += "|";
        m_sFlushLogStr += sLogInfo;
        
        if(m_sFlushLogStr.size() > 1024 *3)
        {
            PrintFlushLog();
        }
        
        return;
    }

    void TMdbChangeNotify::LogRecdNotifyInfo(const MDB_INT64 iStartSeq, const MDB_INT64 iEndSeq, const MDB_INT64 iCurSeq, const char cActionType, const char* psTabName, std::string & sRecdStr)
    {
        TADD_FUNC("Start.");
        
        static char sTmpInfo[MAX_BLOB_LEN] = {0};
        memset(sTmpInfo, 0, sizeof(sTmpInfo));
        
        snprintf(sTmpInfo, sizeof(sTmpInfo), "StartSeq=[%lld], EndSeq=[%lld],CurSeq=[%lld],ActionType=[%c],TableName=[%s]",iStartSeq, iEndSeq,iCurSeq,cActionType,psTabName);
        TADD_DETAIL("%s", sTmpInfo);

        sRecdStr += sTmpInfo;
    }

    void TMdbChangeNotify::LogRecdPkInfo(std::string & sRecdStr, const char* psPkName, const char* psPkValue )
    {
        TADD_FUNC("Start.");
        
        sRecdStr += ",";
        sRecdStr += psPkName;
        sRecdStr +="=";
        sRecdStr += psPkValue;    
    }

//}

