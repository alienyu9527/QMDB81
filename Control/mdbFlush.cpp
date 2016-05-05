/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbFlush .cpp
*@Description��mdbˢ��ģ��
*@Author:		jin.shaohua
*@Date��	    2012.06
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
#define QUERY_NO_ROLLBACK 0x01   //��Query����ع�
#define QUERY_NO_ORAFLUSH 0x02  //��Query������Oracleˢ��
#define QUERY_NO_REDOFLUSH 0x04 //��Query��������redo��־
#define QUERY_NO_SHARDFLUSH 0x08 // ��Query�������ɷ�Ƭ����ͬ��



    TCOLUMN_POS::TCOLUMN_POS()
    {
        Clear();
    }

    TCOLUMN_POS::~TCOLUMN_POS()
    {

    }
    //����
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
    * ��������	:  IsSame
    * ��������	:  �ж��Ƿ�һ��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TCOLUMN_POS::IsSame(int iSqlType,std::vector<ST_COLUMN_VALUE> & vColList,std::vector<ST_COLUMN_VALUE> & vWhereList)
    {
        if(iSqlType != m_iSqlType)
        {
            return false;
        }
        if(TK_DELETE == iSqlType)
        {
            return true;   //ɾ���϶�һ��
        }
        int iCount = 0;
        std::vector<ST_COLUMN_VALUE>::iterator itor = vColList.begin();
        for(; itor != vColList.end(); ++itor)
        {
            if(itor->iColPos != m_pColumnPos[iCount])
            {
                //��ƥ��
                return false;
            }
            iCount ++;
        }
        if(iCount < 50 && m_pColumnPos[iCount] != -1)
        {
            return false;   //���滹��ֵ
        }
        iCount = 0;
        itor = vWhereList.begin();
        for(; itor != vWhereList.end(); ++itor)
        {
            if(itor->iColPos != m_pWherePos[iCount])
            {
                //��ƥ��
                return false;
            }
            iCount ++;
        }
        if(iCount < 10 && m_pWherePos[iCount] != -1)
        {
            return false;   //���滹��ֵ
        }
        return true;
    }

    /******************************************************************************
    * ��������	:  SetPos
    * ��������	:
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TCOLUMN_POS::SetPos(int iSqlType,std::vector<ST_COLUMN_VALUE> & vColList,std::vector<ST_COLUMN_VALUE> & vWhereList)
    {
        int iRet = 0;
        Clear();//����
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
    * ��������	:  GetFreeColumnPos
    * ��������	:  ��ȡһ�����е�columnPos
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TCOLUMN_POS * TTABLE_SQL_BUF::GetFreeColumnPos()
    {
        TCOLUMN_POS * pFreeColumnPos = NULL;
        //Ѱ��һ�����л���
        int i = 0;
        for(i = 0; i<100; i++)
        {
            if(NULL == m_pColmunPos[i] )
            {
                m_pColmunPos[i] = new TCOLUMN_POS();
                if(NULL == m_pColmunPos[i])
                {
                    //û�пռ�
                    TADD_ERROR(ERR_APP_INVALID_PARAM, "m_pColmunPos[%d] is null",i);
                    return NULL;
                }
                pFreeColumnPos = m_pColmunPos[i];
                break;
            }
        }
        if(NULL == pFreeColumnPos)
        {
            //���ȡһ��
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
    * ��������	:  TMdbFlush
    * ��������	:  ����MDB����������ˢ��ͬ��������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
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
    * ��������	:  ~TMdbFlush
    * ��������	:  ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbFlush::~TMdbFlush()
    {
		SAFE_DELETE(m_psDataBuff);
		SAFE_DELETE(m_psNameBuff);
		SAFE_DELETE(m_psColmLenBuff);
		SAFE_DELETE(m_psColmValueBuff);
    }
    /******************************************************************************
    * ��������	:  Init
    * ��������	:  ��ʼ��
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:
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

        //����ͬ������
        if(m_pDsn->m_bIsOraRep 
            &&! QueryHasProperty(iFlags,QUERY_NO_ORAFLUSH) 
            && (REP_TO_DB == m_pTable->iRepAttr ))
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_ORA);
        }

        if((m_pTable->m_bShardBack) && !QueryHasProperty(iFlags,QUERY_NO_SHARDFLUSH))
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_SHARD_BACKUP);
        }
        TMdbTableSpace* pTS = pShmDsn->GetTableSpaceAddrByName(m_pTable->m_sTableSpace);
        CHECK_OBJ(pTS);
        if((pTS->m_bFileStorage) && !QueryHasProperty(iFlags,QUERY_NO_REDOFLUSH))
        {
            FlushTypeSetProperty(m_iFlushType,FLUSH_REDO);
        }

        /*
        if(m_pDsn->m_bIsRep && !QueryHasProperty(iFlags,QUERY_NO_REPFLUSH) &&
        (Column_To_Rep == m_pTable->tColumn[0].iRepAttr ||
        Column_Ora_Rep == m_pTable->tColumn[0].iRepAttr))
        {
        FlushTypeSetProperty(m_iFlushType,FLUSH_REP);
        }
        if(m_pConfig->GetDSN()->bIsPeerRep && !QueryHasProperty(iFlags,QUERY_NO_PEERFLUSH) && 
        (Column_To_Rep == m_pTable->tColumn[0].iRepAttr ||
        Column_Ora_Rep == m_pTable->tColumn[0].iRepAttr))
        {
        FlushTypeSetProperty(m_iFlushType,FLUSH_PEER);
        }*/

        /*
        if(m_pDsn->m_bIsCaptureRouter)
        {
        //����·��
        TMdbMemQueue * pCapQueue = (TMdbMemQueue * )pShmDsn->GetSyncAreaShm(SA_CAPTURE);
        CHECK_OBJ(pCapQueue);
        m_CaptureQueueCtrl.Init(pCapQueue,m_pDsn);
        }
        //����ͬ������
        if(!QueryHasProperty(iFlags,QUERY_NO_REDOFLUSH) && 
        NULL != pShmDsn->GetSyncAreaShm(SA_REDO))
        {
        TMdbMemQueue * pRedoQueue = (TMdbMemQueue *)pShmDsn->GetSyncAreaShm(SA_REDO);
        m_RedoQueueCtrl.Init(pRedoQueue,m_pDsn);
        FlushTypeSetProperty(m_iFlushType,FLUSH_REDO);
        }
        if(m_pDsn->m_bIsOraRep 
        &&! QueryHasProperty(iFlags,QUERY_NO_ORAFLUSH) 
        && (Column_To_Ora == m_pTable->tColumn[0].iRepAttr 
        || Column_Ora_Rep == m_pTable->tColumn[0].iRepAttr))
        {
        FlushTypeSetProperty(m_iFlushType,FLUSH_ORA);
        }
        if(m_pDsn->m_bIsRep && !QueryHasProperty(iFlags,QUERY_NO_REPFLUSH) &&
        (Column_To_Rep == m_pTable->tColumn[0].iRepAttr ||
        Column_Ora_Rep == m_pTable->tColumn[0].iRepAttr))
        {
        FlushTypeSetProperty(m_iFlushType,FLUSH_REP);
        }
        if(m_pConfig->GetDSN()->bIsPeerRep && !QueryHasProperty(iFlags,QUERY_NO_PEERFLUSH) && 
        (Column_To_Rep == m_pTable->tColumn[0].iRepAttr ||
        Column_Ora_Rep == m_pTable->tColumn[0].iRepAttr))
        {
        FlushTypeSetProperty(m_iFlushType,FLUSH_PEER);
        }
        */

        /*if(m_pTable->m_bShardBack)
        {
        FlushTypeSetProperty(m_iFlushType,FLUSH_SHARD_BACKUP);
        }*/

        /*
        if(FlushTypeHasProperty(m_iFlushType,FLUSH_ORA))
        {
        m_pOraQueue = (TMdbMemQueue*)pShmDsn->GetSyncAreaShm(SA_ORACLE);
        CHECK_OBJ(m_pOraQueue);
        CHECK_RET(m_OraQueueCtrl.Init(m_pOraQueue, m_pDsn),"Init failed.");
        }
        */
        //UR: 415794
        //if(FlushTypeHasProperty(m_iFlushType,FLUSH_REP))
        /*
        if(FlushTypeHasAnyProperty(m_iFlushType,FLUSH_REP|FLUSH_PEER))
        {
        m_pRepQueue = (TMdbMemQueue*)pShmDsn->GetSyncAreaShm(SA_REP);
        CHECK_OBJ(m_pRepQueue);
        CHECK_RET(m_RepQueueCtrl.Init(m_pRepQueue, m_pDsn),"Init failed");
        }

        if(m_pTable->m_bShardBack)
        //if(m_pTable->m_bShardBack && !QueryHasProperty(iFlags,QUERY_NO_SHARDFLUSH))
        {
        FlushTypeSetProperty(m_iFlushType,FLUSH_SHARD_BACKUP);
        m_pShardBackQueue = (TMdbMemQueue*)pShmDsn->GetSyncAreaShm(SA_SHARD_BACKUP);
        CHECK_OBJ(m_pShardBackQueue);
        CHECK_RET(m_ShardBackQueueCtrl.Init(m_pShardBackQueue, m_pDsn),"Init failed");
        }
        */
        m_pQueue = (TMdbMemQueue*)pShmDsn->GetSyncAreaShm();
        CHECK_OBJ(m_pQueue);
        CHECK_RET(m_QueueCtrl.Init(m_pQueue, m_pDsn),"Init failed");
        //memset(m_sDataBuf,0x00,sizeof(m_sDataBuf));
        TADD_FUNC("Finish.m_iFlushType=[%d].",m_iFlushType);
        return iRet;
    }

    /******************************************************************************
    * ��������	:  bNeedToFlush
    * ��������	:  �Ƿ���Ҫflush
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbFlush::bNeedToFlush()
    {
        ST_SQL_STRUCT & stSqlStruct = m_pMdbSqlParser->m_stSqlStruct;
        bool bRet = false;
        do
        {
            if(stSqlStruct.pMdbTable->bIsSysTab)
            {
                bRet = false;//dba ����Ҫͬ��
                break;
            }

            if(stSqlStruct.iSqlType == TK_SELECT)
            {
                //��ѯ����Ҫͬ��
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
    * ��������	:  FlushData
    * ��������	:  ˢ������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*
    int TMdbFlush::FlushData()
    {
    TADD_FUNC("Start.");
    int iRet = 0;
    if(!bNeedToFlush())
    {
    return iRet;
    }
    m_sDataBuf[0] = '\0';
    int iLen = 0;
    CHECK_RET(FillData(m_sDataBuf,iLen),"FillData failed.");
    TADD_DETAIL("m_sDataBuf=[%s],size=[%d]",m_sDataBuf,iLen);
    switch(m_pMdbSqlParser->m_iSourceId)
    {
    case SOURCE_APP://��oracle������ͬ��
    if(FlushTypeHasProperty(m_iFlushType,FLUSH_ORA))
    {
    CHECK_RET(FlushToBuf(&m_OraQueueCtrl,m_sDataBuf,iLen),"FlushToBuf faild.");
    }
    if(FlushTypeHasAnyProperty(m_iFlushType,FLUSH_REP|FLUSH_PEER))
    {
    CHECK_RET(FlushToBuf(&m_RepQueueCtrl,m_sDataBuf,iLen),"FlushToBuf faild.");
    }
    break;
    case SOURCE_REP://�򱸻�ͬ��
    case SOURCE_PEER:
    if(FlushTypeHasAnyProperty(m_iFlushType,FLUSH_REP|FLUSH_PEER))
    {
    CHECK_RET(FlushToBuf(&m_RepQueueCtrl,m_sDataBuf,iLen),"FlushToBuf faild.");
    }
    break;
    default:
    break;
    }
    TADD_FUNC("Finish.");
    return iRet;
    }
    */




    /******************************************************************************
    * ��������	:  GetSourceId
    * ��������	:  ��ȡsource id ������ͬ��������ʹ��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    //int TMdbFlush::GetSourceId()
    //{
    //    return m_pMdbSqlParser->m_iSourceId + 1;
    //}

    /******************************************************************************
    * ��������	:  FlushToOraBuf
    * ��������	:  ������ˢ��ora����
    * ����		:  sTemp -  ���� iLen-���ݳ���
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbFlush::FlushToBuf(TMdbQueue* pMemQueue,char * const sTemp,int iLen)
    {
        int iRet =0;
        CHECK_OBJ(pMemQueue);
        //���push����ȥ
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
        // ���ȵȼ�¼������������
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
        //memset(m_sNameBuff, 0, sizeof(m_sNameBuff));
        //memset(m_sColmLenBuff, 0, sizeof(m_sColmLenBuff));
        //memset(m_sColmValueBuff, 0, sizeof(m_sColmValueBuff));
		

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

        // ��ֵ����
        //SAFESTRCPY(sTemp +iPos, strlen(m_sColmLenBuff), m_sColmLenBuff);
        sprintf(sTemp +iPos,"%s", m_psColmLenBuff);
        iPos += strlen(m_psColmLenBuff);

        // ��ֵ
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
    * ��������	:  InsertIntoQueue
    * ��������	:  ���ݲ��뵽ͬ������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:dong.chun
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



    TConfigShmWrap::TConfigShmWrap():
    m_pShmDSN(NULL),
        m_pConfig(NULL)
    {

    }
    TConfigShmWrap::~TConfigShmWrap()
    {

    }
    /******************************************************************************
    * ��������	:  Init
    * ��������	:  ��ʼ��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TConfigShmWrap::Init(TMdbConfig * pConfig,TMdbShmDSN * pShmDSN)
    {
        m_pConfig = pConfig;
        m_pShmDSN = pShmDSN;
        return 0;
    }
    /******************************************************************************
    * ��������	:  GetTableById
    * ��������	:  ����ID��ȡtable
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
            //�Ϸ�table
            return pRetTable;
        }
        return NULL;
    }
//}

