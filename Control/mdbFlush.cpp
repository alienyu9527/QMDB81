/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbFlush .cpp
*@Description：mdb刷新模块
*@Author:		jin.shaohua
*@Date：	    2012.06
*@History:
******************************************************************************************/
#include "Control/mdbFlush.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbSQLParser.h"
#include <ctime>
#include <cstdlib>
#include "Helper/mdbRepRecd.h"

//namespace QuickMDB{
#define QUERY_NO_ROLLBACK 0x01   //该Query无需回滚
#define QUERY_NO_ORAFLUSH 0x02  //该Query无需向Oracle刷新
#define QUERY_NO_REDOFLUSH 0x04 //该Query无需生成redo日志
#define QUERY_NO_SHARDFLUSH 0x08 // 该Query无需生成分片备份同步



    TCOLUMN_POS::TCOLUMN_POS()
    {
        Clear();
    }

    TCOLUMN_POS::~TCOLUMN_POS()
    {

    }
    //清理
    void TCOLUMN_POS::Clear()
    {
        for(int i = 0; i<50; i++)
        {
            m_pColumnPos[i] = -1;
        }

        for(int i = 0; i<10; i++)
        {
            m_pWherePos[i] = -1;
        }
        memset(m_sSQL,0,sizeof(m_sSQL));
        m_iSqlType = -1;
        m_iColumnCount = 0;
        m_iWhereCount = 0;
    }
    /******************************************************************************
    * 函数名称	:  IsSame
    * 函数描述	:  判断是否一样
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TCOLUMN_POS::IsSame(int iSqlType,std::vector<ST_COLUMN_VALUE> & vColList,std::vector<ST_COLUMN_VALUE> & vWhereList)
    {
        if(iSqlType != m_iSqlType)
        {
            return false;
        }
        if(TK_DELETE == iSqlType)
        {
            return true;   //删除肯定一样
        }
        int iCount = 0;
        std::vector<ST_COLUMN_VALUE>::iterator itor = vColList.begin();
        for(; itor != vColList.end(); ++itor)
        {
            if(itor->iColPos != m_pColumnPos[iCount])
            {
                //不匹配
                return false;
            }
            iCount ++;
        }
        if(iCount < 50 && m_pColumnPos[iCount] != -1)
        {
            return false;   //后面还有值
        }
        iCount = 0;
        itor = vWhereList.begin();
        for(; itor != vWhereList.end(); ++itor)
        {
            if(itor->iColPos != m_pWherePos[iCount])
            {
                //不匹配
                return false;
            }
            iCount ++;
        }
        if(iCount < 10 && m_pWherePos[iCount] != -1)
        {
            return false;   //后面还有值
        }
        return true;
    }

    /******************************************************************************
    * 函数名称	:  SetPos
    * 函数描述	:
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TCOLUMN_POS::SetPos(int iSqlType,std::vector<ST_COLUMN_VALUE> & vColList,std::vector<ST_COLUMN_VALUE> & vWhereList)
    {
        int iRet = 0;
        Clear();//清理
        m_iSqlType = iSqlType;
        std::vector<ST_COLUMN_VALUE>::iterator itor = vColList.begin();
        int iCount = 0;
        for(; itor != vColList.end(); ++itor)
        {
            m_pColumnPos[iCount] = itor->iColPos;
            iCount ++;
        }
        m_iColumnCount = iCount;
        itor = vWhereList.begin();
        iCount = 0;
        for(; itor != vWhereList.end(); ++itor)
        {
            m_pWherePos[iCount] = itor->iColPos;
            iCount ++;
        }
        m_iWhereCount= iCount;
        return iRet;
    }

    TTABLE_SQL_BUF::TTABLE_SQL_BUF()
    {
        for(int i = 0; i< 100; i++)
        {
            m_pColmunPos[i] = NULL;
        }
    }

    TTABLE_SQL_BUF::~TTABLE_SQL_BUF()
    {
        for(int i = 0; i<100; i++)
        {
            SAFE_DELETE(m_pColmunPos[i]);
        }
    }
    /******************************************************************************
    * 函数名称	:  GetFreeColumnPos
    * 函数描述	:  获取一个空闲的columnPos
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TCOLUMN_POS * TTABLE_SQL_BUF::GetFreeColumnPos()
    {
        TCOLUMN_POS * pFreeColumnPos = NULL;
        //寻找一个空闲缓存
        int i = 0;
        for(i = 0; i<100; i++)
        {
            if(NULL == m_pColmunPos[i] )
            {
                m_pColmunPos[i] = new TCOLUMN_POS();
                if(NULL == m_pColmunPos[i])
                {
                    //没有空间
                    TADD_ERROR(ERR_APP_INVALID_PARAM, "m_pColmunPos[%d] is null",i);
                    return NULL;
                }
                pFreeColumnPos = m_pColmunPos[i];
                break;
            }
        }
        if(NULL == pFreeColumnPos)
        {
            //随机取一个
            srand((unsigned)time(0));
            pFreeColumnPos = m_pColmunPos[rand()%100];
            if(NULL != pFreeColumnPos)
            {
                pFreeColumnPos->Clear();
            }
        }
        return pFreeColumnPos;
    }


    /******************************************************************************
    * 函数名称	:  TMdbFlush
    * 函数描述	:  将对MDB操作的数据刷到同步缓存中
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbFlush::TMdbFlush():
    m_pMdbSqlParser(NULL),
        m_pConfig(NULL),
        m_pQueue(NULL),
        m_pDsn(NULL),
        m_pShardBackQueue(NULL),
        m_pTable(NULL),
        m_iFlushType(0),
        m_psDataBuff(NULL),
        m_psNameBuff(NULL),
        m_psColmLenBuff(NULL),
        m_psColmValueBuff(NULL)
        
    {

    }
    /******************************************************************************
    * 函数名称	:  ~TMdbFlush
    * 函数描述	:  析构
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbFlush::~TMdbFlush()
    {
		SAFE_DELETE(m_psDataBuff);
		SAFE_DELETE(m_psNameBuff);
		SAFE_DELETE(m_psColmLenBuff);
		SAFE_DELETE(m_psColmValueBuff);
    }
    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbFlush::Init(TMdbSqlParser * pMdbSqlParser,MDB_INT32 iFlags)
    {
        TADD_FUNC("Start.iFlag=%d",iFlags);
        int iRet = 0;
        m_iFlushType = 0;
        m_pMdbSqlParser = pMdbSqlParser;
        CHECK_OBJ(m_pMdbSqlParser);
        m_pTable = pMdbSqlParser->m_stSqlStruct.pMdbTable;
        TMdbShmDSN * pShmDsn = pMdbSqlParser->m_stSqlStruct.pShmDSN;
        m_pDsn = pShmDsn->GetInfo();
        CHECK_OBJ(m_pDsn);
        m_pConfig = pMdbSqlParser->m_pMdbConfig;
        CHECK_OBJ(m_pConfig);
        CHECK_RET(m_tRowCtrl.Init(m_pDsn->sName,m_pMdbSqlParser->m_stSqlStruct.pMdbTable->sTableName),"m_tRowCtrl.Init");

        //设置同步属性
        if(m_pDsn->m_bIsOraRep 
            &&! QueryHasProperty(iFlags,QUERY_NO_ORAFLUSH) 
            && (REP_TO_DB == m_pTable->iRepAttr ))
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_ORA);
        }

        if((m_pTable->m_bShardBack) && !QueryHasProperty(iFlags,QUERY_NO_SHARDFLUSH) && m_pConfig->GetDSN()->m_bIsShardBackup)
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_SHARD_BACKUP);
        }
        TMdbTableSpace* pTS = pShmDsn->GetTableSpaceAddrByName(m_pTable->m_sTableSpace);
        CHECK_OBJ(pTS);
        if((pTS->m_bFileStorage) && !QueryHasProperty(iFlags,QUERY_NO_REDOFLUSH))
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_REDO);
        }

        m_pQueue = (TMdbMemQueue*)pShmDsn->GetSyncAreaShm();
        CHECK_OBJ(m_pQueue);
        CHECK_RET(m_QueueCtrl.Init(m_pQueue, m_pDsn),"Init failed");
        TADD_FUNC("Finish.m_iFlushType=[%d].",m_iFlushType);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  bNeedToFlush
    * 函数描述	:  是否需要flush
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbFlush::bNeedToFlush()
    {
        ST_SQL_STRUCT & stSqlStruct = m_pMdbSqlParser->m_stSqlStruct;
        bool bRet = false;
        do
        {
            if(stSqlStruct.pMdbTable->bIsSysTab)
            {
                bRet = false;//dba 表不需要同步
                break;
            }

            if(stSqlStruct.iSqlType == TK_SELECT)
            {
                //查询不需要同步
                bRet = false;
                break;
            }


            if(FlushTypeHasAnyProperty(m_iFlushType, FLUSH_ORA|FLUSH_SHARD_BACKUP|FLUSH_REDO))
            {
                bRet = true;
                break;
            }
            else
            {
                bRet = false;
                break;
            }
        }
        while(0);
        TADD_DETAIL("NeedToFlush = [%s].",bRet?"TRUE":"FALSE");
        return bRet;
    }

    /******************************************************************************
    * 函数名称	:  FlushToOraBuf
    * 函数描述	:  将数据刷到ora缓存
    * 输入		:  sTemp -  数据 iLen-数据长度
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbFlush::FlushToBuf(TMdbQueue* pMemQueue,char * const sTemp,int iLen)
    {
        int iRet =0;
        CHECK_OBJ(pMemQueue);
        //如果push不进去
        if(pMemQueue->Push(sTemp, iLen) == false)
        {
            TADD_WARNING("flushData wait for 0.03 seconds,not enough memory to flush.");
            //iRet = ERROR_NO_MEMORY_TO_FLUSH;
        }
        return iRet;
    }

    int TMdbFlush::FillRepData(char * sTemp,long long iPageLSN, long long iTimeStamp,char cVersion,int & iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iPos = 0;

        // ^^
        sTemp[0] = '^';
        sTemp[1] = '^';
        iPos += 2;

        // record Length [4 bytes]
        // 长度等记录完整后再设置
        iPos += 4;

        // Version [1 bytes]
        sTemp[6] = cVersion; 
        iPos += 1;

        // flush flag [4 bytes]
        //sTemp[7] = GetSourceId() + '0';
        sprintf(sTemp +iPos , "%04d", m_iFlushType);
        iPos += 4;

        // LSN [20 bytes]
        sprintf(sTemp +iPos , "%20lld", iPageLSN);
        iPos += 20;

        // TimeStamp [10 bytes]
        sprintf(sTemp +iPos , "%10lld", iTimeStamp);
        iPos += 10;

        // RowID [8 bytes]
        sprintf(sTemp +iPos , "%08lld", m_pMdbSqlParser->m_stSqlStruct.iRowPos);
        iPos += 8;

        // SQL Type [2 bytes]
        sprintf(sTemp +iPos , "%02d", m_pMdbSqlParser->m_stSqlStruct.iSqlType);
        iPos += 2;

        // Routing ID [4 bytes]
        if(m_pMdbSqlParser->m_stSqlStruct.pstRouterID == NULL)
        {
            TADD_DETAIL("no routing id table!");
            sprintf(sTemp +iPos , "%04d", DEFALUT_ROUT_ID);
        }
        else
        {
            TADD_DETAIL("set routing id =[%lld]", m_pMdbSqlParser->m_stSqlStruct.pstRouterID->pExprValue->lValue);
            sprintf(sTemp +iPos , "%04lld", m_pMdbSqlParser->m_stSqlStruct.pstRouterID->pExprValue->lValue);
        }        
        iPos += 4;

        m_iColmCount = 0;
        m_iNamePos = 0;
        m_iColumLenPos = 0;
        m_iColumValuePos = 0;

		if(NULL == m_psColmValueBuff)
        {
            m_psColmValueBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psColmValueBuff)
            {

                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
				return -1;
            }
        }
        else
        {

            m_psColmValueBuff[0]='\0';
        }

		if(NULL == m_psNameBuff)
        {
            m_psNameBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psNameBuff)
            {

                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
                return -1;
            }
        }
        else
        {

            m_psNameBuff[0]='\0';
        }

		if(NULL == m_psColmLenBuff)
        {
            m_psColmLenBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psColmLenBuff)
            {

                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
                return -1;
            }
        }
        else
        {

            m_psColmLenBuff[0]='\0';
        }
        sprintf(m_psNameBuff, "%s",m_pTable->sTableName);
        m_iNamePos += strlen(m_psNameBuff);

        switch(m_pMdbSqlParser->m_stSqlStruct.iSqlType)
        {
        case TK_INSERT:
            SetRepColmData(m_pMdbSqlParser->m_listOutputCollist);
            break;
        case TK_UPDATE:
            SetRepColmData(m_pMdbSqlParser->m_listOutputCollist);
            m_psNameBuff[m_iNamePos] = '|';
            m_iNamePos++;
            SetRepColmData(m_pMdbSqlParser->m_listInputPriKey);
            break;
        case TK_DELETE:
            m_psNameBuff[m_iNamePos] = '|';
            m_iNamePos++;
            SetRepColmData(m_pMdbSqlParser->m_listInputPriKey);
            break;
        default:
            CHECK_RET(-1,"sqltype[%d] do not need to flush.",
                m_pMdbSqlParser->m_stSqlStruct.iSqlType);
            break;
        }

        // Column Name Length [4 bytes]
        sprintf(sTemp +iPos , "%04d", m_iNamePos);
        iPos += 4;

        //SAFESTRCPY(sTemp +iPos, strlen(m_sNameBuff), m_sNameBuff);
        sprintf(sTemp +iPos,"%s", m_psNameBuff);
        iPos += strlen(m_psNameBuff);

        // Column count[4 bytes]
        sprintf(sTemp +iPos , "%04d", m_iColmCount);
        iPos += 4;

        // 列值长度
        //SAFESTRCPY(sTemp +iPos, strlen(m_sColmLenBuff), m_sColmLenBuff);
        sprintf(sTemp +iPos,"%s", m_psColmLenBuff);
        iPos += strlen(m_psColmLenBuff);

        // 列值
        //SAFESTRCPY(sTemp +iPos, strlen(m_sColmValueBuff), m_sColmValueBuff);
        sprintf(sTemp +iPos,"%s", m_psColmValueBuff);
        iPos += strlen(m_psColmValueBuff);

        // ##
        sTemp[iPos] = '#';
        iPos++;
        sTemp[iPos] = '#';
        iPos++;

        iLen = iPos;

        sTemp[2] = (iLen)/1000 + '0';
        sTemp[3] = ((iLen)%1000)/100 + '0';
        sTemp[4] = ((iLen)%100)/10 + '0';
        sTemp[5] = ((iLen)%10) + '0';

        TADD_DETAIL("record:[%s]",sTemp );
        TADD_FUNC("Finish.");
        return iRet;
    }


    int TMdbFlush::SetRepColmData(ST_MEM_VALUE_LIST & stMemValueList)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        int iValueLen = 0;
        ST_MEM_VALUE * pstMemValue = NULL;

        for(int i = 0; i<stMemValueList.iItemNum; ++i)
        {
            pstMemValue = stMemValueList.pValueArray[i];
            CHECK_OBJ(pstMemValue->pColumnToSet);

            if(m_psNameBuff[m_iNamePos-1] == '|')
            {
                sprintf(m_psNameBuff+m_iNamePos,"%s", pstMemValue->pColumnToSet->sName);
                m_iNamePos += strlen(pstMemValue->pColumnToSet->sName);
            }
            else
            {
                sprintf(m_psNameBuff+m_iNamePos,",%s", pstMemValue->pColumnToSet->sName);
                m_iNamePos += (strlen(pstMemValue->pColumnToSet->sName )+ 1);
            }


            m_iColmCount++;

            if(pstMemValue->IsNull())
            {
                sprintf(m_psColmLenBuff + m_iColumLenPos, "%04d", NULL_VALUE_LEN);
                m_iColumLenPos += 4;

            }
            else
            {
                if(MemValueHasProperty(pstMemValue,MEM_Int))
                {
                    sprintf(m_psColmValueBuff + m_iColumValuePos, "%lld", pstMemValue->lValue);
                    //m_iColumValuePos += iValueLen;
                }
                else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                {
                    sprintf(m_psColmValueBuff + m_iColumValuePos, "%s", pstMemValue->sValue);
                }

                iValueLen = strlen(m_psColmValueBuff ) - m_iColumValuePos;
                m_iColumValuePos += iValueLen;

                sprintf(m_psColmLenBuff + m_iColumLenPos, "%04d", iValueLen);
                m_iColumLenPos += 4;
            }

        }

        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  InsertIntoQueue
    * 函数描述	:  数据插入到同步缓存
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:dong.chun
    *******************************************************************************/
    int TMdbFlush::InsertIntoQueue(MDB_INT64 iLsn,long long iTimeStamp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(!bNeedToFlush())
        {
            return iRet;
        }
       // m_sDataBuf[0] = '\0';
        if(NULL == m_psDataBuff)
        {
            m_psDataBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psDataBuff)
            {

                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
                return -1;
            }
        }
        else
        {

            m_psDataBuff[0]='\0';
        }
		  
        int iLen = 0;
        CHECK_RET(FillRepData(m_psDataBuff,iLsn,iTimeStamp,VERSION_DATA_SYNC,iLen),"FillData failed.");
        CHECK_RET(FlushToBuf(&m_QueueCtrl,m_psDataBuff,iLen),"FlushToBuf faild.");
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbFlush::InsertIntoCapture(MDB_INT64 iLsn,long long iTimeStamp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(!m_pDsn->m_bIsCaptureRouter)
        {
            return iRet;
        }

        if(!m_pMdbSqlParser->IsCurRowNeedCapture())
        {
            return iRet;
        }

        //m_sDataBuf[0] = '\0';
        if(NULL == m_psDataBuff)
        {
            m_psDataBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psDataBuff)
            {

                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
                return -1;
            }
        }
        else
        {

            m_psDataBuff[0]='\0';
        }
        int iLen = 0;
        CHECK_RET(FillRepData(m_psDataBuff,iLsn,iTimeStamp,VERSION_DATA_CAPTURE,iLen),"FillData failed.");
        CHECK_RET(FlushToBuf(&m_QueueCtrl,m_psDataBuff,iLen),"FlushToBuf failed.");
        TADD_FUNC("Finish.");
        return iRet;

    }



	/******************************************************************************
    * 函数名称	:  TMdbFlush
    * 函数描述	:  将对MDB操作的数据刷到同步缓存中
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbFlushTrans::TMdbFlushTrans(): 
        m_pQueue(NULL),
        m_pDsn(NULL),
        m_pShardBackQueue(NULL),
        m_pTable(NULL),
        m_iFlushType(0),
        m_psDataBuff(NULL),
        m_psNameBuff(NULL),
        m_psColmLenBuff(NULL),
        m_psColmValueBuff(NULL)
        
    {
		m_llRoutingID = DEFALUT_ROUT_ID;
    }
    /******************************************************************************
    * 函数名称	:  ~TMdbFlush
    * 函数描述	:  析构
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbFlushTrans::~TMdbFlushTrans()
    {
		SAFE_DELETE(m_psDataBuff);
		SAFE_DELETE(m_psNameBuff);
		SAFE_DELETE(m_psColmLenBuff);
		SAFE_DELETE(m_psColmValueBuff);
    }
    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:
    *******************************************************************************/
    int TMdbFlushTrans::Init(TMdbShmDSN * pShmDSN,TMdbTable*  pTable, int iSqlType,char* pDataAddr)
    {
        int iRet = 0;
        m_iFlushType = 0;
		m_iSqlType = iSqlType;
        m_pTable = pTable;
		m_pDataAddr = pDataAddr;
        TMdbShmDSN * pShmDsn = pShmDSN;
        m_pDsn = pShmDsn->GetInfo();
        CHECK_OBJ(m_pDsn);
        CHECK_RET(m_tRowCtrl.Init(m_pDsn->sName,pTable->sTableName),"m_tRowCtrl.Init");

		m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pDsn->sName);
		
        //设置同步属性
        if(m_pDsn->m_bIsOraRep && (REP_TO_DB == m_pTable->iRepAttr ))
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_ORA);
        }

        if(m_pTable->m_bShardBack && m_pConfig->GetDSN()->m_bIsShardBackup)
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_SHARD_BACKUP);
        }
        TMdbTableSpace* pTS = pShmDsn->GetTableSpaceAddrByName(m_pTable->m_sTableSpace);
        CHECK_OBJ(pTS);
        if(pTS->m_bFileStorage)
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_REDO);
        }

        m_pQueue = (TMdbMemQueue*)pShmDsn->GetSyncAreaShm();
        CHECK_OBJ(m_pQueue);
        CHECK_RET(m_QueueCtrl.Init(m_pQueue, m_pDsn),"Init failed");
        TADD_FUNC("Finish.m_iFlushType=[%d].",m_iFlushType);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  bNeedToFlush
    * 函数描述	:  是否需要flush
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbFlushTrans::bNeedToFlush()
    {
        bool bRet = false;
        do
        {
            if(m_pTable->bIsSysTab)
            {
                bRet = false;//dba 表不需要同步
                break;
            }

            if(m_iSqlType == TK_SELECT)
            {
                //查询不需要同步
                bRet = false;
                break;
            }


            if(FlushTypeHasAnyProperty(m_iFlushType, FLUSH_ORA|FLUSH_SHARD_BACKUP|FLUSH_REDO))
            {
                bRet = true;
                break;
            }
            else
            {
                bRet = false;
                break;
            }
        }
        while(0);
        TADD_DETAIL("NeedToFlush = [%s].",bRet?"TRUE":"FALSE");
        return bRet;
    }

	bool  TMdbFlushTrans::IsCurRowNeedCapture()
	{
		m_llRoutingID = GetRoutingID();
		if(DEFALUT_ROUT_ID == m_llRoutingID){return false;}
	    if(TK_SELECT == m_iSqlType){return false;}//select 不捕获
	    if(false == m_pDsn->m_bIsCaptureRouter){return false;}//不需要捕获
	    int * arrRouterToCap = m_pDsn->m_arrRouterToCapture;
	    if(NULL == arrRouterToCap){return false;}
	    int i = 0;
	    long long llRowRouter = m_llRoutingID;
	    for(i = 1;i <= arrRouterToCap[0];++i)
	    {
	        if(llRowRouter == arrRouterToCap[i])
	        {
	            return true;
	        }
	    }
	    return false;
	}

    /******************************************************************************
    * 函数名称	:  FlushToOraBuf
    * 函数描述	:  将数据刷到ora缓存
    * 输入		:  sTemp -  数据 iLen-数据长度
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbFlushTrans::FlushToBuf(TMdbQueue* pMemQueue,char * const sTemp,int iLen)
    {
        int iRet =0;
        CHECK_OBJ(pMemQueue);
        //如果push不进去
        if(pMemQueue->Push(sTemp, iLen) == false)
        {
            TADD_WARNING("flushData wait for 0.03 seconds,not enough memory to flush.");
            //iRet = ERROR_NO_MEMORY_TO_FLUSH;
        }
        return iRet;
    }

	long long TMdbFlushTrans::GetRoutingID()
	{	
		if(m_llRoutingID != DEFALUT_ROUT_ID)
		{
			return m_llRoutingID;
		}
		
		long long llValue = DEFALUT_ROUT_ID;
		char* sValue = NULL;
		int iValueType = 0;
		for(int i = 0;i < m_pTable->iColumnCounts;++i)
	    {
	        if(TMdbNtcStrFunc::StrNoCaseCmp(m_pTable->tColumn[i].sName,m_pConfig->GetDSN()->sRoutingName) == 0)
	        {//有路由信息列
				m_tRowCtrl.GetOneColumnValue(m_pDataAddr,&m_pTable->tColumn[i],llValue,sValue,-1,iValueType);
	            break;
	        }
	    }
		m_llRoutingID = llValue;
		
		return  llValue;
	}

    int TMdbFlushTrans::FillRepData(char * sTemp,long long iPageLSN, long long iTimeStamp,int & iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iPos = 0;

        // ^^
        sTemp[0] = '^';
        sTemp[1] = '^';
        iPos += 2;

        // record Length [4 bytes]
        // 长度等记录完整后再设置
        iPos += 4;

        // Version [1 bytes]
        sTemp[6] = 'S'; 
        iPos += 1;

        // flush flag [4 bytes]
        //sTemp[7] = GetSourceId() + '0';
        sprintf(sTemp +iPos , "%04d", m_iFlushType);
        iPos += 4;

        // LSN [20 bytes]
        sprintf(sTemp +iPos , "%20lld", iPageLSN);
        iPos += 20;

        // TimeStamp [10 bytes]
        sprintf(sTemp +iPos , "%10lld", iTimeStamp);
        iPos += 10;

        // RowID [8 bytes]
        sprintf(sTemp +iPos , "%08lld", 0);
        iPos += 8;

        // SQL Type [2 bytes]
        sprintf(sTemp +iPos , "%02d", m_iSqlType);
        iPos += 2;

        // Routing ID [4 bytes]        
        sprintf(sTemp +iPos , "%04d", GetRoutingID());
           
        iPos += 4;

        m_iColmCount = 0;
        m_iNamePos = 0;
        m_iColumLenPos = 0;
        m_iColumValuePos = 0;

		if(NULL == m_psColmValueBuff)
        {
            m_psColmValueBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psColmValueBuff)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
				return -1;
            }
        }
        else
        {
            m_psColmValueBuff[0]='\0';
        }

		if(NULL == m_psNameBuff)
        {
            m_psNameBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psNameBuff)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
                return -1;
            }
        }
        else
        {
           m_psNameBuff[0]='\0';
        }

		if(NULL == m_psColmLenBuff)
        {
            m_psColmLenBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psColmLenBuff)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
                return -1;
            }
        }
        else
        {
            m_psColmLenBuff[0]='\0';
        }
        sprintf(m_psNameBuff, "%s",m_pTable->sTableName);
        m_iNamePos += strlen(m_psNameBuff);

        switch(m_iSqlType)
        {
        case TK_INSERT:
            SetRepColmDataAll();
            break;
        case TK_UPDATE:
			SetRepColmDataAll();
            m_psNameBuff[m_iNamePos] = '|';
            m_iNamePos++;
            SetRepColmDataPK();
            break;
        case TK_DELETE:
            m_psNameBuff[m_iNamePos] = '|';
            m_iNamePos++;
			SetRepColmDataPK();
            break;
        default:
            CHECK_RET(-1,"sqltype[%d] do not need to flush.",m_iSqlType);
            break;
        }

        // Column Name Length [4 bytes]
        sprintf(sTemp +iPos , "%04d", m_iNamePos);
        iPos += 4;

        sprintf(sTemp +iPos,"%s", m_psNameBuff);
        iPos += strlen(m_psNameBuff);

        // Column count[4 bytes]
        sprintf(sTemp +iPos , "%04d", m_iColmCount);
        iPos += 4;

        // 列值长度
        sprintf(sTemp +iPos,"%s", m_psColmLenBuff);
        iPos += strlen(m_psColmLenBuff);

        // 列值
        sprintf(sTemp +iPos,"%s", m_psColmValueBuff);
        iPos += strlen(m_psColmValueBuff);

        // ##
        sTemp[iPos] = '#';
        iPos++;
        sTemp[iPos] = '#';
        iPos++;

        iLen = iPos;

        sTemp[2] = (iLen)/1000 + '0';
        sTemp[3] = ((iLen)%1000)/100 + '0';
        sTemp[4] = ((iLen)%100)/10 + '0';
        sTemp[5] = ((iLen)%10) + '0';

        TADD_DETAIL("record:[%s]",sTemp );
        TADD_FUNC("Finish.");
        return iRet;
    }

	bool TMdbFlushTrans::CheckIsPK(int iPos)
	{
		for(int i = 0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
        {	
        	if(iPos == m_pTable->m_tPriKey.iColumnNo[i]) return true;
		}
		return false;
	}

	int TMdbFlushTrans::SetRepColmDataAll()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        int iValueLen = 0;
		
		//同步所有的列
        for(int i = 0; i<m_pTable->iColumnCounts; ++i)
        {	
        	
        	//update不更新主键,不同步
        	if( (m_iSqlType==TK_UPDATE) && CheckIsPK(m_pTable->tColumn[i].iPos))
			{
				continue;
			}

			
        	char* sColName = m_pTable->tColumn[i].sName;
            if(m_psNameBuff[m_iNamePos-1] == '|')
            {
                sprintf(m_psNameBuff+m_iNamePos,"%s", sColName);
                m_iNamePos += strlen(sColName);
            }
            else
            {
                sprintf(m_psNameBuff+m_iNamePos,",%s", sColName);
                m_iNamePos += (strlen(sColName)+ 1);
            }
            m_iColmCount++;
		
			long long llValue = 0;
			char* sValue = NULL;
			int iValueType = 0;
			m_tRowCtrl.GetOneColumnValue(m_pDataAddr,&m_pTable->tColumn[i],llValue,sValue,-1,iValueType);
				
			if(MEM_Null == iValueType)
            {
                sprintf(m_psColmLenBuff + m_iColumLenPos, "%04d", NULL_VALUE_LEN);
                m_iColumLenPos += 4;

            }
            else
            {
                if(MEM_Int == iValueType)
                {
                    sprintf(m_psColmValueBuff + m_iColumValuePos, "%lld", llValue);
                }
                else if(MEM_Str == iValueType)
                {
                    sprintf(m_psColmValueBuff + m_iColumValuePos, "%s", sValue);
                }

                iValueLen = strlen(m_psColmValueBuff ) - m_iColumValuePos;
                m_iColumValuePos += iValueLen;

                sprintf(m_psColmLenBuff + m_iColumLenPos, "%04d", iValueLen);
                m_iColumLenPos += 4;
            }

        }

        TADD_FUNC("Finish.");
        return iRet;
    }
	
    int TMdbFlushTrans::SetRepColmDataPK()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        int iValueLen = 0;
		
		//添加所有的主键
        for(int i = 0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
        {	
        	int iPKColumnPos = m_pTable->m_tPriKey.iColumnNo[i];
			char* sPKName = m_pTable->tColumn[iPKColumnPos].sName;
			
            if(m_psNameBuff[m_iNamePos-1] == '|')
            {
                sprintf(m_psNameBuff+m_iNamePos,"%s", sPKName);
                m_iNamePos += strlen(sPKName);
            }
            else
            {
                sprintf(m_psNameBuff+m_iNamePos,",%s", sPKName);
                m_iNamePos += (strlen(sPKName)+ 1);
            }
            m_iColmCount++;
		
			long long llValue = 0;
			char* sValue = NULL;
			int iValueType = 0;
			m_tRowCtrl.GetOneColumnValue(m_pDataAddr,&m_pTable->tColumn[iPKColumnPos],llValue,sValue,-1,iValueType);
				
			if(MEM_Null == iValueType)
            {
                sprintf(m_psColmLenBuff + m_iColumLenPos, "%04d", NULL_VALUE_LEN);
                m_iColumLenPos += 4;

            }
            else
            {
                if(MEM_Int == iValueType)
                {
                    sprintf(m_psColmValueBuff + m_iColumValuePos, "%lld", llValue);
                }
                else if(MEM_Str == iValueType)
                {
                    sprintf(m_psColmValueBuff + m_iColumValuePos, "%s", sValue);
                }

                iValueLen = strlen(m_psColmValueBuff ) - m_iColumValuePos;
                m_iColumValuePos += iValueLen;

                sprintf(m_psColmLenBuff + m_iColumLenPos, "%04d", iValueLen);
                m_iColumLenPos += 4;
            }

        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    
    int TMdbFlushTrans::MakeBuf(MDB_INT64 iLsn,long long iTimeStamp)
    {
		TADD_FUNC("Start.");
        int iRet = 0;
        if( !bNeedToFlush() && !IsCurRowNeedCapture())
		{
			return iRet;
		}
		
        if(NULL == m_psDataBuff)
        {
            m_psDataBuff = new char[MAX_VALUE_LEN];
            if(NULL == m_psDataBuff)
            {

                TADD_ERROR(ERR_OS_NO_MEMROY,"no memory to string for syn buffer data");
                return -1;
            }
        }
        else
        {
            m_psDataBuff[0]='\0';
        }
		  
        CHECK_RET(FillRepData(m_psDataBuff,iLsn,iTimeStamp,m_iBufLen),"FillData failed.");
		return iRet;
	}

	void TMdbFlushTrans::SetBufVersion(char cVersion)
	{
		m_psDataBuff[6]=cVersion;
	}
	
	
    int TMdbFlushTrans::InsertBufIntoQueue()
    {  
        TADD_FUNC("Start.");
        int iRet = 0;	

		if(!bNeedToFlush())
		{
			return iRet;
		}
		
    	SetBufVersion(VERSION_DATA_SYNC);
        CHECK_RET(FlushToBuf(&m_QueueCtrl,m_psDataBuff,m_iBufLen),"FlushToBuf faild.");
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbFlushTrans::InsertBufIntoCapture()
    {
        TADD_FUNC("Start.");
        int iRet = 0;	
		
		if(!IsCurRowNeedCapture())
        {
            return iRet;
        }
		
    	SetBufVersion(VERSION_DATA_CAPTURE);	
        CHECK_RET(FlushToBuf(&m_QueueCtrl,m_psDataBuff,m_iBufLen),"FlushToBuf failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }


    TConfigShmWrap::TConfigShmWrap():
    m_pShmDSN(NULL),
        m_pConfig(NULL)
    {

    }
    TConfigShmWrap::~TConfigShmWrap()
    {

    }
    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TConfigShmWrap::Init(TMdbConfig * pConfig,TMdbShmDSN * pShmDSN)
    {
        m_pConfig = pConfig;
        m_pShmDSN = pShmDSN;
        return 0;
    }
    /******************************************************************************
    * 函数名称	:  GetTableById
    * 函数描述	:  根据ID获取table
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbTable * TConfigShmWrap::GetTableByName(const char* psTableName)
    {
        TMdbTable * pRetTable = NULL;
        if(NULL == psTableName)
        {
            return NULL;
        }
        if(NULL == m_pShmDSN && NULL != m_pConfig)
        {
            TADD_FLOW("GetTable from config");
            pRetTable = m_pConfig->GetTableByName(psTableName);
        }
        else if(NULL != m_pShmDSN)
        {
            TADD_FLOW("GetTable from Shm");
            pRetTable = m_pShmDSN->GetTableByName(psTableName);
        }
        if(NULL != pRetTable && pRetTable->IsValidTable())
        {
            //合法table
            return pRetTable;
        }
        return NULL;
    }
//}

