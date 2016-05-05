/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbExecuteEngine.cpp
*@Description�� SQLִ������
*@Author:	     jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/
#include "Control/mdbExecuteEngine.h"
#include "Control/mdbVarcharMgr.h"
#include "Helper/mdbDateTime.h"
#include "Helper/SyntaxTreeAnalyse.h"
//#include "Helper/mdbPerfInfo.h"
#include "Interface/mdbRollback.h"
#include "Helper/mdbMemValue.h"
#include "Helper/mdbBase.h"
#include "Helper/mdbMalloc.h"
//#include "BillingSDK.h"


//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

#define CHECK_RET_FILL(_ret,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);m_tError.FillErrMsg(iRet,__VA_ARGS__);return iRet;}
#define CHECK_RET_FILL_CODE(_ret,_code,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);m_tError.FillErrMsg(_code,__VA_ARGS__);return _code;}
#define CHECK_OBJ_FILL(_obj) if(NULL == _obj){TADD_ERROR(ERR_APP_INVALID_PARAM,#_obj" is null");m_tError.FillErrMsg(ERR_OS_NO_MEMROY,#_obj" is null");return ERR_APP_INVALID_PARAM;}
#define CHECK_RET_FILL_BREAK(_ret,...)if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);m_tError.FillErrMsg(iRet,__VA_ARGS__);break;}
#define CHECK_RET_FILL_CODE_BREAK(_ret,_code,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);iRet=_code;m_tError.FillErrMsg(_code,__VA_ARGS__);break;}

    //next ������
    enum E_NEXT_TYPE
    {
        NEXT_NORMAL  = 1, //����������������next
        NEXT_CACHE    = 2//�����ѯ
    };

    /******************************************************************************
    * ��������	:  TMdbExecuteEngine
    * ��������	:  mdbִ�������ʼ������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbExecuteEngine::TMdbExecuteEngine():
        m_pMdbSqlParser(NULL),
        m_iCurIndex(-1),
        m_bScanAll(false),
        m_llScanAllPos(-1),
        m_pTable(NULL),
        m_pDsn(NULL),
        m_iRowsAffected(0),
        m_iIsStop(0),
        m_pInsertBlock(NULL),
        m_pVarCharBlock(NULL),
        m_pRollback(NULL),
        m_pUpdateBlock(NULL),
        m_aRowIndexPos(NULL)
    {
        memset(m_sTempValue,0x00,sizeof(m_sTempValue));
    }

    /******************************************************************************
    * ��������	:  ~TMdbExecuteEngine
    * ��������	:  mdbִ����������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbExecuteEngine::~TMdbExecuteEngine()
    {
        SAFE_DELETE_ARRAY(m_pInsertBlock);
        SAFE_DELETE_ARRAY(m_pVarCharBlock);
        SAFE_DELETE_ARRAY(m_pUpdateBlock);
        SAFE_DELETE_ARRAY(m_aRowIndexPos);
    }
    /******************************************************************************
    * ��������	:  Init
    * ��������	:  ��һЩ��ʼ����������
    * ����		:  pMdbSqlParser -������iFlag - SQL����
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::Init(TMdbSqlParser * pMdbSqlParser,MDB_INT32 iFlag)
    {
        TADD_FUNC("Start.iFlag=[%d].",iFlag);
        int iRet = 0;
        SAFE_DELETE_ARRAY(m_pInsertBlock);//����insertblock��Ҫÿ��������Ϊ��¼���Ȳ�һ��
        SAFE_DELETE_ARRAY(m_pUpdateBlock);
        SAFE_DELETE_ARRAY(m_aRowIndexPos);//����
        CHECK_OBJ_FILL(pMdbSqlParser);
        m_pMdbSqlParser = pMdbSqlParser;
        ClearLastExecute();
        m_pTable = m_pMdbSqlParser->m_stSqlStruct.pMdbTable;
        TADD_DETAIL("Execute Table=[%s].",m_pTable->sTableName);
        m_pDsn   = m_pMdbSqlParser->m_stSqlStruct.pShmDSN->GetInfo();
        m_mdbPageCtrl.SetDSN(m_pDsn->sName);
        //m_tVarcharMgr.SetConfig(m_pMdbSqlParser->m_stSqlStruct.pShmDSN);//����varchar
        m_tVarcharCtrl.Init(m_pDsn->sName);
        CHECK_RET_FILL_CODE(m_mdbTSCtrl.Init(m_pDsn->sName,m_pTable->m_sTableSpace),
                       ERR_OS_ATTACH_SHM,"m_mdbTSCtrl.Init error");
        CHECK_RET_FILL_CODE(m_mdbIndexCtrl.AttachTable(m_pMdbSqlParser->m_stSqlStruct.pShmDSN,m_pMdbSqlParser->m_stSqlStruct.pMdbTable),
                       ERR_OS_ATTACH_SHM,"m_mdbIndexCtrl.AttachTable failed.");
        CHECK_RET_FILL_CODE(m_MdbTableWalker.AttachTable(m_pMdbSqlParser->m_stSqlStruct.pShmDSN,m_pMdbSqlParser->m_stSqlStruct.pMdbTable),
                       ERR_OS_ATTACH_SHM,"m_MdbTableWalker.AttachTable failed.");
        CHECK_RET_FILL_CODE(m_tMdbFlush.Init(pMdbSqlParser,iFlag),ERR_SQL_FLUSH_DATA,"m_tMdbFlush.Init failed");//��ʼ��flushģ��
        CHECK_RET_FILL_CODE(m_tObserveTableExec.Init(m_pDsn->sName,OB_TABLE_EXEC),ERR_DB_OBSERVE,"m_tObserveTableExec.Init failed");
        CHECK_RET_FILL_CODE(m_tObserveTableExec.SetExecEngine(this),ERR_DB_OBSERVE,"m_tObserveTableExec.SetExecEngine failed.");
        CHECK_RET_FILL_CODE(m_tObserveTableExec.SetSQLParser(pMdbSqlParser),ERR_DB_OBSERVE,"m_tObserveTableExec.SetSQLParser failed.");
        CHECK_RET_FILL_CODE(m_tRowCtrl.Init(m_pDsn->sName,m_pTable->sTableName),ERR_OS_ATTACH_SHM,"m_tRowCtrl.AttachTable failed.");//��¼����
        CHECK_RET_FILL_CODE(m_tCacheTable.Create(pMdbSqlParser),iRet,"Cache Table Create failed.");//������ʱ��
        TADD_FUNC("Finish.");
        return iRet;
    }


	int TMdbExecuteEngine::CheckDiskFree()
	{
		int iRet = 0;

		if (!m_tMdbFlush.bNeedToFlush())
		{
			return iRet;
		}
		
		if(m_pDsn->bDiskSpaceStat == false)
		{
			return ERR_OS_NO_DISK_SPACE;
		}

		return iRet;
	}

    /******************************************************************************
    * ��������	:  Execute
    * ��������	:  ����������SQL�ṹ���н���
    * ����		:
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 - ���ɹ�
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::Execute()
    {
        TADD_FUNC("Start.");
        int iRet = 0;	
        ClearLastExecute();//������һ�ε�ִ�н��
        CHECK_RET_FILL(CheckDiskFree(),"DiskSpace < [%lld]KB, Cannot Flush log, Execute failed.",MIN_DISK_SPACE_KB);
        //����ܷ����Ķ��������Ƿ����
        CHECK_RET_FILL(m_pMdbSqlParser->CheckTableExist(),"Table[%s] not exist",m_pMdbSqlParser->m_stSqlStruct.sTableName);
        CHECK_RET_FILL(m_pMdbSqlParser->GetWhereIndex(m_pVIndex),"GetWhereIndex error.");//��ȡ����ʹ�õ�����
        CHECK_OBJ(m_pVIndex);
        if(m_pVIndex->size() == 0)
        {
            TADD_DETAIL("no index, use all scan....");//���û��ʹ��������ȫ������
            m_bScanAll = true;
            m_llScanAllPos = -1;
        }
        m_pMdbSqlParser->ExecuteLimit();//����limit��Χ
        switch(m_pMdbSqlParser->m_stSqlStruct.iSqlType)
        {
            //��SQL����ִ��
        case TK_INSERT:
            CHECK_RET_FILL(ExecuteInsert(),"ExecuteInsert failed");
            break;
        case TK_DELETE:
            CHECK_RET_FILL(m_tLimitCtrl.Init(m_pMdbSqlParser->m_listOutputLimit),"m_tLimitCtrl.Init error..");
            CHECK_RET_FILL(ExecuteDelete(),"ExecuteDelete failed");
            break;
        case TK_UPDATE:
            CHECK_RET_FILL(m_tLimitCtrl.Init(m_pMdbSqlParser->m_listOutputLimit),"m_tLimitCtrl.Init error..");
            CHECK_RET_FILL(ExecuteUpdate(),"ExecuteUpdate failed");
            break;
        case TK_SELECT:
            m_pMdbSqlParser->ClearMemValue(m_pMdbSqlParser->m_listOutputCollist);//���������
            if(m_tCacheTable.bNeedCached())
            {//��Ҫ�����ѯ
                ExecuteCacheSelect();
                m_tCacheTable.Open();
            }
            CHECK_RET_FILL(m_tLimitCtrl.Init(m_pMdbSqlParser->m_listOutputLimit),"m_tLimitCtrl.Init error..");
            break;
        default:
            CHECK_RET_FILL(-1,"error sql type[%d]",m_pMdbSqlParser->m_stSqlStruct.iSqlType);
            break;
        }
        m_tObserveTableExec.Record();//��¼
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  ExecuteUpdate
    * ��������	:  ִ��update����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteUpdate()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_iRowsAffected = 0;
        //������ʱ���¿�
        if(NULL == m_pUpdateBlock)
        {
            m_pUpdateBlock = new(std::nothrow) char[m_pTable->iOneRecordSize];
            CHECK_OBJ_FILL(m_pUpdateBlock);
            TADD_FLOW("Create new Update Block.size[%d].",m_pTable->iOneRecordSize);
        }
        //�Ƿ���Ҫ�ӱ���
        //bool bNeedLockTable = (0 != m_pMdbSqlParser->m_stSqlStruct.vIndexChanged.size())?true:false;
        bool bNext = false;
        
        long long iTimeStamp = 0;
        iTimeStamp = m_pMdbSqlParser->GetTimeStamp();
        if(iTimeStamp <= 0)
        {
            iTimeStamp = TMdbDateTime::StringToTime(m_pDsn->sCurTime);
        }
        
        while(1)
        {
            
            CHECK_RET_FILL_BREAK(Next(bNext),"Next failed.");//��һ��
            if(false == bNext){break;}//����
            TADD_DETAIL("Find Row[%d].",m_iRowsAffected);
            if(m_iIsStop == 1)
            {
                //�յ��źſ���
                CHECK_RET_FILL(ERR_SQL_STOP_EXEC,"Catch the SIGINT signal.");
            }
            //�ȼӱ���
            /*if(bNeedLockTable)
            {//���ڲ��޸�����������ӱ���
                CHECK_RET_FILL(m_pTable->tTableMutex.Lock(m_pTable->bWriteLock,  &m_pDsn->tCurTime),"tTableMutex lock failed.");
            }*/
            CHECK_RET_FILL_CODE(m_mdbPageCtrl.Attach(m_pPageAddr, m_pTable->bReadLock, m_pTable->bWriteLock),
                ERR_OS_ATTACH_SHM,"tPageCtrl.Attach() failed.");
            CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
            do{
                CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputPriKey),"FillSqlParserValue failed.");//���������Ϣ
                CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue error.");
                CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteSQL(),"ExecuteSQL error.");
                CHECK_RET_FILL_BREAK(GetUpdateDiff(),"GetUpdateDiff error.");
                CHECK_RET_FILL_BREAK(PushRollbackData(m_pDataAddr,m_pUpdateBlock),"PushRollbackData failed...");//ѹ��ع���
                CHECK_RET_FILL_BREAK(UpdateData(),"UpdateData failed..");
                CHECK_RET_FILL_BREAK(UpdateRowDataTimeStamp(m_pDataAddr,m_pTable->m_iTimeStampOffset,iTimeStamp), "update row data timestamp failed.");
            }while(0);
            m_mdbPageCtrl.UnWLock();//��ҳ��
            /*if(bNeedLockTable)
            {//���ڲ��޸�����������ӱ���
                m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);//�����
            }*/
            CHECK_RET_FILL(iRet,"ERROR.");
            m_mdbTSCtrl.SetPageDirtyFlag(((TMdbPage*)m_pPageAddr)->m_iPageID);//������ҳ
            m_iRowsAffected ++;
            CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoQueue(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoQueue failed[%d].",iRet);
            CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoCapture(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoCapture failed[%d].",iRet);
        }
        TADD_FUNC("Finish.");
        return iRet;

    }

    /******************************************************************************
    * ��������	:  ExecuteInsert
    * ��������	:  ִ��insert����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 - ���ɹ�
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteInsert()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //CHECK_RET_FILL(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue[InputCollist] error.");
        CHECK_RET_FILL(m_pMdbSqlParser->ExecuteSQL(),"ExecuteSQL error.");
        //���ڴ����ı����û������������������
        if(m_pTable->iIndexCounts == 0)
        {
            TADD_ERROR(ERR_TAB_INDEX_NOT_EXIST,"Table[%s] doesn't have any index,Insert operations are not allowed.",m_pTable->sTableName);
            return ERR_TAB_INDEX_NOT_EXIST;
        }
        if(m_pMdbSqlParser->m_stSqlStruct.bCheckInsertPriKey)
        {
            //�������
            TADD_DETAIL("Need to Check Primary Key.");
            CHECK_RET_FILL(IsPKExist(),"IsPKExist failed..");
        }
        if(NULL == m_pInsertBlock)
        {
            m_pInsertBlock = new(std::nothrow) char[m_pTable->iOneRecordSize];
            CHECK_OBJ_FILL(m_pInsertBlock);
            TADD_FLOW("Create new Insert Block.size[%d].",m_pTable->iOneRecordSize);
        }
        memset(m_pInsertBlock,0x00,m_pTable->iOneRecordSize);
        //m_pTable->tTableMutex.Lock(m_pTable->bWriteLock,&m_pDsn->tCurTime);//�ӱ���
        
        long long iTimeStamp = m_pMdbSqlParser->GetTimeStamp();
        if(iTimeStamp <= 0)
        {
            iTimeStamp = TMdbDateTime::StringToTime(m_pDsn->sCurTime);
        }

        
        do
        {
            CHECK_RET_FILL_BREAK(InsertDataFill(m_pInsertBlock),"InsertDataFill failed....");//�������
            // ���ü�¼ʱ���
            CHECK_RET_FILL_BREAK(SetRowDataTimeStamp(m_pInsertBlock, m_pTable->m_iTimeStampOffset,iTimeStamp),"insert row data timestamp failed.");
            CHECK_RET_FILL_BREAK(PushRollbackData(m_pInsertBlock,NULL),"PushRollbackData failed...");//ѹ��ع����ݳɹ�����ܸ����ں�����
            CHECK_RET_FILL_BREAK(InsertData(m_pInsertBlock,m_pTable->iOneRecordSize),"InsertData failed");//���뵽�ڴ���
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue failed.");//���������Ҫ��������
        }
        while(0);
        //m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);
        CHECK_RET_FILL(iRet,"ERROR.");
        ++m_iRowsAffected;
        CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoQueue(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoQueue failed[%d].",iRet);
        CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoCapture(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoCapture failed[%d].",iRet);
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * ��������	:  ExecuteDelete
    * ��������	:  ִ��ɾ��
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteDelete()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_iRowsAffected = 0;
        //m_pTable->tTableMutex.Lock(m_pTable->bWriteLock, &m_pDsn->tCurTime);//�ӱ���
        int i = 0;
        bool bNext = false;
        while(1)
        {
            CHECK_RET_FILL_BREAK(Next(bNext),"next failed.");
            if(false == bNext){break;}
            TADD_DETAIL("Find Row[%d].",m_iRowsAffected);
            if(m_iIsStop == 1)
            {
                //�յ��źſ���
                CHECK_RET_FILL_BREAK(ERR_SQL_STOP_EXEC,"Catch the SIGINT signal.");
            }
            CHECK_RET_FILL_BREAK(PushRollbackData(m_pDataAddr,NULL),"PushRollbackData failed...");//ѹ��ع�����
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputPriKey),"FillSqlParserValue failed.");//���������Ϣ
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue failed.");//���������Ҫ��������
            //TMdbRowIndex * arrRowIndex = GetRowIndexArray(m_pDataAddr);
            //ɾ������
            for(i=0; i<m_pTable->iIndexCounts; ++i)
            {
                CHECK_RET_FILL_BREAK(m_mdbIndexCtrl.DeleteIndexNode(i,m_pDataAddr,m_tRowCtrl,m_tCurRowIDData),
                    "DeleteIndexNode[%d] failed.",i);
            }
            CHECK_RET_FILL_BREAK(iRet,"CalcIndexValue failed.");
            //ɾ��varchar����
            do
            {
                CHECK_RET_FILL_CODE_BREAK(DeleteVarCharValue(m_pDataAddr),ERR_SQL_INDEX_COLUMN_ERROR,"DeleteVarCharValue error.");
            }
            while(0);
            CHECK_RET_FILL_BREAK(iRet,"iRet = [%d].",iRet);
            //ɾ���ڴ��е�����
            TADD_DETAIL("Delete From Page[%d]",m_iPagePos);
            CHECK_RET_FILL_BREAK(m_mdbPageCtrl.Attach(m_pPageAddr, m_pTable->bReadLock, m_pTable->bWriteLock),
                            "m_mdbPageCtrl.Attach faild");
            CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
            CHECK_RET_FILL_BREAK(m_mdbPageCtrl.DeleteData_NoMutex(m_iPagePos),"m_mdbPageCtrl.DeleteData");
            m_mdbTSCtrl.SetPageDirtyFlag(((TMdbPage*)m_pPageAddr)->m_iPageID);//������ҳ
            m_mdbPageCtrl.UnWLock();//��ҳ��
            //���ڱ��е�ҳ����
            TMdbPage *pPage = (TMdbPage*)m_pPageAddr;
            if(pPage->bNeedToMoveToFreeList() && TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState, "full") == 0)
            {
                //�������ɾ��֮��,�ռ��㹻��(���ÿռ�>=ҳ���С��25%)�������Free-Page����
                CHECK_RET_FILL_BREAK(m_mdbTSCtrl.TablePageFullToFree(m_pTable,pPage),"TablePageFullToFree error");
            }
            m_pTable->tTableMutex.Lock(m_pTable->bWriteLock, &m_pDsn->tCurTime);
            --m_pTable->iCounts; //����һ����¼
            m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);
            m_iRowsAffected ++;
            CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoQueue(((TMdbPage *)m_pPageAddr)->m_iPageLSN,TMdbDateTime::StringToTime(m_pDsn->sCurTime)),ERR_SQL_FLUSH_DATA,"InsertIntoQueue failed[%d].",iRet);
            CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoCapture(((TMdbPage *)m_pPageAddr)->m_iPageLSN,TMdbDateTime::StringToTime(m_pDsn->sCurTime)),ERR_SQL_FLUSH_DATA,"InsertIntoCapture failed[%d].",iRet);
        }
        //m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  InsertDataFill
    * ��������	:  ���ڲ����ֵ���ڴ�ֲ��������
    * ����		:  ��
    * ���		:  sTmp - �������ڴ�ռ�
    * ����ֵ	:  0 - �ɹ�!0 - ���ɹ�
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::InsertDataFill(char* const &sTmp)
    {
        TADD_FUNC("Start.sTmp=[%p].",sTmp);
        int iRet = 0;
        int i    = 0;
        int iSize = m_pMdbSqlParser->m_listOutputCollist.iItemNum;
        TADD_DETAIL("Column counts[%d].",iSize);
        ST_MEM_VALUE ** pMemValueArray = m_pMdbSqlParser->m_listOutputCollist.pValueArray;
        for(i = 0; i < iSize; i++)
        {
            CHECK_RET_FILL(m_tRowCtrl.FillOneColumn(sTmp,pMemValueArray[i]->pColumnToSet,pMemValueArray[i],TK_INSERT),
                        "FillOneColumn[%s] error,value=[%s]",pMemValueArray[i]->pColumnToSet->sName,pMemValueArray[i]->ToString().c_str());
        }

        
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  InsertData
    * ��������	:  �����ݲ��뵽�ڴ���
    �������ݿ���ֱ�Ӱѱ���ס, ��Ϊ�϶���Ҫ��������/��ȡ����ҳ/���±���Ϣ����Щ����Ҫ����

    * ����		:  pAddr - ���ݵ�ַ��iSize - ���ݴ�С
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::InsertData(char* pAddr, int iSize)
    {
        TADD_FUNC("(iSize=%d) : Start.", iSize);
        TMdbRowID rowID;
        int iRet = 0;
        while(true)
        {
            if(m_iIsStop == 1)
            {
                CHECK_RET_FILL(ERR_SQL_FILL_MDB_INFO,"Catch the SIGINT signal.");
            }
            TMdbPage * pFreePage= NULL;
            CHECK_RET_FILL(m_mdbTSCtrl.GetFreePage(m_pTable,pFreePage),"GetFreePage failed.");//�ҵ�һ������ҳ
            //�����ݷ���ҳ����
            CHECK_RET_FILL_CODE(m_mdbPageCtrl.Attach((char *)pFreePage, m_pTable->bReadLock, m_pTable->bWriteLock),
                           ERR_OS_ATTACH_SHM,"Can't Attach to page.");
            CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
            iRet = m_mdbPageCtrl.InsertData_NoMutex((unsigned char*)pAddr, iSize, rowID,m_pDataAddr);
            if(iRet == 0)
            {
                m_mdbTSCtrl.SetPageDirtyFlag(pFreePage->m_iPageID);
            }
            m_mdbPageCtrl.UnWLock();//��ҳ��
            TADD_DETAIL("Get one record:DataAddr[%p],CurRowData[%d|%d],PageAddr[%p],PagePos[%d].",
            m_pDataAddr,rowID.GetPageID(),rowID.GetDataOffset(),
            (char*)pFreePage,rowID.GetDataOffset());

            if(ERR_PAGE_NO_MEMORY == iRet)
            {
                //�����ҵ�������ҳ���˶��޷��������ݣ�������һ������ҳ
                TADD_DETAIL("Current page is Full.");
                CHECK_RET_FILL_CODE(m_mdbTSCtrl.TablePageFreeToFull(m_pTable,pFreePage),
                               ERR_OS_NO_MEMROY,"FreeToFull() error.iRet=[%d]",iRet);
                continue;
            }
            m_pPageAddr = (char *)pFreePage;
            break;
        }
        TADD_DETAIL("InsertData(%s) : Data insert OK,rowID=%p .",m_pTable->sTableName,&rowID);
        CHECK_RET_FILL(ChangeInsertIndex(pAddr, rowID),"ChangeInsertIndex failed...");
        m_pTable->tTableMutex.Lock(m_pTable->bWriteLock,&m_pDsn->tCurTime);
        ++m_pTable->iCounts;//���¼������1
        m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);
        TADD_FUNC("Finish.");
        return 0;
    }


    /******************************************************************************
    * ��������	:  UpdateData
    * ��������	:  �����ݸ��µ�
    * ����		:
    * ����		:
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::UpdateData()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //�ȸ�������
        CHECK_RET_FILL(ChangeUpdateIndex(m_pMdbSqlParser->m_stSqlStruct.vIndexChanged),"ChangeUpdateIndex failed.");
        //CHECK_RET_FILL_CODE(m_mdbPageCtrl.Attach(m_pPageAddr, m_pTable->bReadLock, m_pTable->bWriteLock),ERR_OS_ATTACH_SHM,"tPageCtrl.Attach() failed.");
        //CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");

        do
        {
            int i = 0;
            for(i = 0; i<m_pMdbSqlParser->m_listOutputCollist.iItemNum; i++)
            {
                ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listOutputCollist.pValueArray[i];
                CHECK_RET_FILL_BREAK(m_tRowCtrl.FillOneColumn(m_pDataAddr,pstMemValue->pColumnToSet,pstMemValue,TK_UPDATE),"FillOneColumn error");
            }

            
        }
        while(0);
        m_mdbPageCtrl.UpdatePageLSN();//����LSN
        //m_mdbPageCtrl.UnWLock();
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  ChangeInsertIndex
    * ��������	:  ��������(����ʱ)
    * ����		:  pAddr - ���ݵ�ַ���������ݵ�rowid
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ChangeInsertIndex( char* pAddr, TMdbRowID& rowID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //����������Ϣ
        for(int i=0; i<m_pTable->iIndexCounts; ++i)
        {
            TADD_DETAIL("rowID.iPageID=%d, rowID.iDataOffset=%d", rowID.GetPageID(), rowID.GetDataOffset());
            CHECK_RET_FILL(m_mdbIndexCtrl.InsertIndexNode(i,pAddr,m_tRowCtrl,rowID),
                "InsertIndexNode failed.index=[%s]",m_pTable->tIndex[i].sName);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  ChangeUpdateIndex
    * ��������	:  ��������(����ʱ)
    * ����		:  vMemValue - update��set��ֵ,ֻ��Ҫ����update�漰���������о�����
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ChangeUpdateIndex(std::vector<ST_INDEX_VALUE > & vUpdateIndex)
    {
        TADD_FUNC("Start.vUpdateIndex.size=%d.",vUpdateIndex.size());
        int iRet = 0;
        std::vector<ST_INDEX_VALUE >::iterator itor = vUpdateIndex.begin();
        for(; itor != vUpdateIndex.end(); ++itor)
        {
            ST_INDEX_VALUE & stIndexValue = (*itor);
            do
            {
                CHECK_RET_FILL_CODE_BREAK(m_mdbIndexCtrl.UpdateIndexNode(stIndexValue.pstTableIndex->iIndexPos,
                                     m_pDataAddr,stIndexValue, m_tRowCtrl,m_tCurRowIDData),ERR_SQL_INDEX_COLUMN_ERROR,"UpdateIndexNode error[%d].",iRet);
            }
            while(0);
        }
        return iRet;
    }

    /******************************************************************************
    * ��������	:  CalcMPIndexValue
    * ��������	:  �������������ֵ
    * ����		:  pAddr - ��¼�ĵ�ַ��iIndexPos - index��λ��
    * ���		:  iError - ������
    * ����ֵ	:  ������
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*long long TMdbExecuteEngine::CalcMPIndexValue( char* pAddr, int iIndexPos, int& iError)
    {
        TADD_FUNC("Start(pAddr=%p,iIndexPos=%d).",pAddr,iIndexPos);
        iError = 0;
        //�����ҵ���������Ӧ������
        m_sTempValue[0] = 0;
        for(int i=0; i<MAX_INDEX_COLUMN_COUNTS; ++i)
        {
            if(m_pTable->tIndex[iIndexPos].iColumnNo[i] < 0)
            {
                break;
            }
            //��ȡ�������������ݵ�ƫ����
            int iCol = m_pTable->tIndex[iIndexPos].iColumnNo[i];
            long long llValue = 0;
            char * sValue = NULL;
            int iValueType = 0;
            iError = m_tRowCtrl.GetOneColumnValue(pAddr,&(m_pTable->tColumn[iCol]),llValue,sValue,-1,iValueType);
            if(iError != 0)
            {
                TADD_ERROR(iError,"GetOneColumnValue failed.");
                return 0;
            }
            switch(iValueType)
            {
                case MEM_Int:
                    sprintf(m_sTempValue,"%s;%lld", m_sTempValue, llValue);
                    break;
                case MEM_Str:
                    sprintf(m_sTempValue,"%s;%s", m_sTempValue, sValue);
                    break;
                case MEM_Null:
                    sprintf(m_sTempValue,"%s;0", m_sTempValue);
                    break;
                default:
                    TADD_ERROR(ERR_SQL_TYPE_INVALID,"iValueType=%d invalid.", iValueType);
                    iError = ERR_SQL_TYPE_INVALID;
                    break;
            }
            TADD_DETAIL("sTempValue=%s.", m_sTempValue);
        }
        long long llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }
    */


    /******************************************************************************
    * ��������	:  CalcOneIndexValue
    * ��������	:  ���㵥��������ֵ
    * ����		:  pAddr - ��¼�ĵ�ַ��iIndexPos - index��λ��
    * ���		:  iError - ������
    * ����ֵ	:  ������
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*long long TMdbExecuteEngine::CalcOneIndexValue( char* pAddr, int iIndexPos, int& iError)
    {
        TADD_FUNC("Start(iIndexPos=%d).", iIndexPos);
        iError = 0;
        int iCol = m_pTable->tIndex[iIndexPos].iColumnNo[0];
        TMdbColumn * pColumn = &(m_pTable->tColumn[iCol]);
        TADD_DETAIL("ColoumName=[%s], offset=%d,iCol=%d",pColumn->sName,pColumn->iOffSet,iCol);
        long long llValue = 0;
        char * sValue = 0;
        int iValueType = 0;
        iError = m_tRowCtrl.GetOneColumnValue(pAddr,pColumn,llValue,sValue,-1,iValueType);
        if(iError != 0)
        {
            TADD_ERROR(iError,"GetOneColumnValue[%s] failed.",pColumn->sName);
            return 0;
        }
        switch(iValueType)
        {
            case MEM_Int://ֱ���û�ȡ����ֵ
                break;
            case MEM_Str:
                llValue = TMdbNtcStrFunc::StrToHash(sValue);
                break;
            case MEM_Null:
                llValue = 0;
                break;
            default:
                TADD_ERROR(ERR_SQL_TYPE_INVALID,"iValueType=%d invalid.", iValueType);
                iError = ERR_SQL_TYPE_INVALID;
                break;
        }
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }
    */

    /******************************************************************************
    * ��������	:  CalcIndexValue
    * ��������	:  �������������ֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*long long TMdbExecuteEngine::CalcIndexValue( char* pAddr, int iIndexPos, int& iError)
    {
        TADD_FUNC("Start(pAddr=%p,iIndexPos=%d).",pAddr,iIndexPos);
        iError = 0;
        long long llValue = 0;
        if(m_pTable->tIndex[iIndexPos].m_iIndexType == HT_CMP)
        {
            llValue = CalcMPIndexValue(pAddr, iIndexPos, iError);
        }
        else
        {
            llValue = CalcOneIndexValue(pAddr, iIndexPos, iError);
        }
        llValue = llValue<0? -llValue:llValue;
        llValue = llValue % m_pTable->iRecordCounts;
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }*/

    /******************************************************************************
    * ��������	:  Next
    * ��������	:  ��ȡ��һ������
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  true - ��һ����ֵ��false - û����һ����
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::Next(bool & bResult)
    {
        TADD_FUNC("Start.next_type=[%d].",m_iNextType);
        int iRet = 0;
        bool bLoop = false;
        do
        {
            bLoop = false;
            switch(m_iNextType)
            {
            case NEXT_NORMAL:
                CHECK_RET(NextWhere(bResult),"NextWhere failed.");
                if(bResult){m_pMdbSqlParser->m_stSqlStruct.iRowPos ++;}
                break;
             #if 0
            case NEXT_MONI:
                bResult =  MoniNext();
                break;
            case NEXT_ORDERBY:
                bResult = OrderbyNext();
                break;
            #endif
            case NEXT_CACHE:
                {
                    bResult = CacheNext();
                }
                break;
            default:
                TADD_ERROR(ERROR_UNKNOWN,"error next type[%d].",m_iNextType);
                break;
            }
            switch(m_tLimitCtrl.DoNext())
            {
            case DO_NEXT_TRUE:
                bLoop = false;
                break;
            case DO_NEXT_CONTINUE:
                bLoop = true;
                break;
            case DO_NEXT_END:
                bLoop = false;
                bResult =  false;
                break;
            default:
                break;
            }
        }
        while(bLoop);
        TADD_FUNC("Finish.iRet[%d],result[%d].",iRet,bResult);
        return iRet;
    }


    /******************************************************************************
    * ��������	:  GetNextIndex
    * ��������	:  ��ȡ��һ������
    * ����		:  ��
    * ���		:  pTableIndex - ������Ϣ��llValue - ����ֵ��Ϣ
    * ����ֵ	:   0 - �ɹ�!0 - ���ɹ�
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::GetNextIndex(ST_TABLE_INDEX_INFO * & pTableIndex,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_bScanAll)
        {
            if(NULL == m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex)
            {
                pTableIndex = NULL;
                return iRet;
            }
        
            // ȫ������
            m_llScanAllPos ++;
            llValue = m_llScanAllPos;
            long long iMaxPos = 0;
            if(m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex->pIndexInfo->m_iAlgoType == INDEX_HASH)
            {
                iMaxPos = m_pTable->iRecordCounts;
            }
            else if(m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex->pIndexInfo->m_iAlgoType == INDEX_M_HASH)
            {
                iMaxPos = m_pTable->iTabLevelCnts;
            }
            else
            {
                iMaxPos = m_pTable->iRecordCounts;
            }
            
            if(m_llScanAllPos == 0 || m_llScanAllPos < iMaxPos)
            {
                pTableIndex = m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex;
            }
            else
            {
                pTableIndex = NULL;
            }
        }
        else
        {
            //����������
            if((int)m_pVIndex->size() - 1 == m_iCurIndex || m_iCurIndex < -1)
            {
                pTableIndex = NULL;
            }
            else
            {
                m_iCurIndex ++;
                pTableIndex = (*m_pVIndex)[m_iCurIndex].pstTableIndex;
                CalcMemValueHash((*m_pVIndex)[m_iCurIndex],llValue);
            }
        }
        if(NULL != pTableIndex)
        {
            TADD_DETAIL("pTableIndex=[%p][%s],llValue=[%lld]",pTableIndex,pTableIndex->pIndexInfo->sName,llValue);
        }
        else
        {
            TADD_DETAIL("pTableIndex=NULL,llValue=[%lld]",llValue);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	
    int TMdbExecuteEngine::SetTrieWord()
   	{
   		int  iRet = 0;
		if((int)m_pVIndex->size() <= 0 || m_iCurIndex < -1)
        {
        	return iRet;
        }
		
		ST_INDEX_VALUE stIndexValue = (*m_pVIndex)[m_iCurIndex];
		
		if(stIndexValue.pstTableIndex->pIndexInfo->m_iAlgoType == INDEX_TRIE && \
			stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType == HT_Char ) 
		{
			SAFESTRCPY(m_MdbTableWalker.m_sTrieWord, MAX_TRIE_WORD_LEN ,stIndexValue.pExprArr[0]->pExprValue->sValue);			
		}
		else
		{
			m_MdbTableWalker.m_sTrieWord[0] = 0;
		}

		return iRet;		
	}
	

    /******************************************************************************
    * ��������	:  CalcMemValueHash
    * ��������	:  ����value��hashֵ
    * ����		:  pMemValue - ����Ľڵ�
    * ���		:   llValue:���ص�hashֵ
    * ����ֵ	:  0 - �ɹ�!0 - ���ɹ�
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbExecuteEngine::CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_sTempValue[0] = 0;
        if(HT_CMP ==  stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
            //�������
            int i = 0;
            for(i = 0; i < MAX_INDEX_COLUMN_COUNTS; ++i)
            {
                if(stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i] < 0)
                {
                    break;
                }
                CHECK_OBJ_FILL(stIndexValue.pExprArr[i]);
                ST_MEM_VALUE * pstMemValue = stIndexValue.pExprArr[i]->pExprValue;
                CHECK_OBJ_FILL(pstMemValue);
                if(pstMemValue->IsNull())
                {
                    sprintf(m_sTempValue, "%s;0",m_sTempValue);
                }
                else if(MemValueHasProperty(pstMemValue,MEM_Int))
                {
                    sprintf(m_sTempValue, "%s;%lld", m_sTempValue, pstMemValue->lValue);
                }
                else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                {
                    sprintf(m_sTempValue, "%s;%s", m_sTempValue, pstMemValue->sValue);
                }
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        }
        else
        {
            //������
            if(stIndexValue.pExprArr[0]->pExprValue->IsNull())
            {
                llValue = 0;
            }
            else  if(MemValueHasAnyProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Str|MEM_Date))
            {
                llValue = TMdbNtcStrFunc::StrToHash(stIndexValue.pExprArr[0]->pExprValue->sValue);
            }
            else if(MemValueHasProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Int))
            {
                llValue = stIndexValue.pExprArr[0]->pExprValue->lValue;
            }
        }
        llValue = llValue<0? -llValue:llValue;
        //����ɢ��
        llValue = llValue % m_pTable->iRecordCounts;//m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iRecordCounts;
        TADD_FUNC("Finish.llValue = %lld.m_sTempValue=[%s].",llValue,m_sTempValue);
        return iRet;
    }
    */

    /******************************************************************************
    * ��������	:  CalcMemValueHash
    * ��������	:  ����value��hashֵ
    * ����		:  pMemValue - ����Ľڵ�
    * ���		:   llValue:���ص�hashֵ
    * ����ֵ	:  0 - �ɹ�!0 - ���ɹ�
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbExecuteEngine::CalcMemValueListHash(ST_MEM_VALUE_LIST & stMemValueList,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(stMemValueList.iItemNum > 1)
        {
            //�������
            m_sTempValue[0] = 0;
            int i = 0;
            for(i = 0; i < stMemValueList.iItemNum; i++)
            {
                ST_MEM_VALUE * pstMemValue = stMemValueList.pValueArray[i];
                if(pstMemValue != NULL)
                {
                    if(pstMemValue->IsNull())
                    {
                        sprintf(m_sTempValue, "%s;0", m_sTempValue);
                    }
                    else if(MemValueHasProperty(pstMemValue,MEM_Int))
                    {
                        sprintf(m_sTempValue, "%s;%lld", m_sTempValue, pstMemValue->lValue);
                    }
                    else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                    {
                        sprintf(m_sTempValue, "%s;%s", m_sTempValue, pstMemValue->sValue);
                    }

                }
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
            TADD_DETAIL("CMP_INDEX:m_sTempValue=[%s],llValue=[%lld]",m_sTempValue,llValue );
        }
        else
        {
            //������
            ST_MEM_VALUE * pstMemValue = stMemValueList.pValueArray[0];
            if(pstMemValue->IsNull())
            {
                llValue = 0;
            }
            else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
            {
                llValue = TMdbNtcStrFunc::StrToHash(pstMemValue->sValue);
            }
            else if(MemValueHasProperty(pstMemValue,MEM_Int))
            {
                llValue = pstMemValue->lValue;
            }
        }
        llValue = llValue<0? -llValue:llValue;
        //����ɢ��
        llValue = llValue % m_pTable->iRecordCounts;//m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iRecordCounts;
        TADD_FUNC("Finish.llValue=%lld.",llValue);
        return iRet;
    }
    */

    /******************************************************************************
    * ��������	:  CheckWhere
    * ��������	:  ���where�����Ƿ�����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  true - ���㣬false - ������
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::CheckWhere(bool &bResult)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_RET(FillSqlParserValue(m_pMdbSqlParser->m_listInputWhere),"FillSqlParserValue where failed.");
        CHECK_RET(m_pMdbSqlParser->ExecuteWhere(bResult),"ExecuteWhere failed.");
        TADD_FUNC("Finish(%s).", bResult?"TRUE":"FALSE");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  FillSqlParserValue
    * ��������	:  ���ֵsql����������Ҫ����ֵ
    * ����		:  vMemValue - ������ֵ
    * ���		:  vMemValue - ���ú��ֵ
    * ����ֵ	:  0 - �ɹ�!0 - ���ɹ�
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::FillSqlParserValue(ST_MEM_VALUE_LIST & stMemValueList)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int i = 0;
        m_pMdbSqlParser->ClearMemValue(stMemValueList);//����
        int iSize = stMemValueList.iItemNum;
        TMdbColumn * pColumn = NULL;
        ST_MEM_VALUE * pMemValue = NULL;
        int iValueType = 0;
        for(i = 0; i<iSize; i++)
        {
            if(NULL == m_pDataAddr){continue;}
            pMemValue = stMemValueList.pValueArray[i];
            pColumn = pMemValue->pColumn;
            TADD_DETAIL("Column[%s],type[%d],offset[%d].",pColumn->sName,pColumn->iDataType,pColumn->iOffSet);
            CHECK_RET(m_tRowCtrl.GetOneColumnValue(m_pDataAddr,pColumn,pMemValue->lValue,pMemValue->sValue,
                pMemValue->iSize,iValueType),"GetOneColumnValue failed.");
            switch(iValueType)
            {
                case MEM_Int:
                case MEM_Str:
                    break;
                case MEM_Null:
                    pMemValue->SetNull();
                    break;
                default:
                    CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,"column[%s] iValueType=%d invalid.",pColumn->sName, iValueType);
                    break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    //ɾ���䳤����
    int TMdbExecuteEngine::DeleteVarCharValue( char*  const &pAddr)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //�ж��Ƿ��б䳤�洢
        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            if(m_pTable->tColumn[i].iDataType == DT_VarChar || m_pTable->tColumn[i].iDataType == DT_Blob)
            {
                int iOffSet = m_pTable->tColumn[i].iOffSet;
                int iWhichPos = -1;
                unsigned int iRowId = 0;
                m_tVarcharCtrl.GetStoragePos(&(pAddr[iOffSet]), iWhichPos, iRowId);
                if(iWhichPos < VC_16 || iWhichPos > VC_8192)
                {
                    continue;
                }
                CHECK_RET(m_tVarcharCtrl.Delete(iWhichPos,iRowId),"Delete Varchar Faild");
            }
        }
        TADD_FUNC("End");
        return iRet;
    }



    /******************************************************************************
    * ��������	:  IsNeedReadLock
    * ��������	:  �Ƿ���Ҫ�Ӷ���:���������ҼӶ���ʱ��ʹ��cache��ʽ next
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool  TMdbExecuteEngine::IsNeedReadLock()
    {
        return (m_pVIndex->size() != 0 && m_pTable->bReadLock);
    }



    /******************************************************************************
    * ��������	:  ExecuteMySelect
    * ��������	:  ִ�в�ѯ����
    				������ѯ����	����ѯ�������������
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteCacheSelect()
    {
       TADD_FUNC("Start.");
        int iRet = 0;
        if(IsNeedReadLock())
        {
            m_pTable->tTableMutex.Lock(m_pTable->bReadLock,&m_pDsn->tCurTime);//�ӱ���
        }
        bool bNext = false;
        TMdbCacheRow * pCacheRow = NULL;

		
		if(m_pMdbSqlParser->m_listOutputGroupBy.iItemNum != 0)
		{
			iRet =m_tCacheTable.CreateHashTable();
			CHECK_RET(iRet,"Create hash table failed");
			
		}
		
        while(1)
        {
            CHECK_RET_FILL_BREAK(Next(bNext),"next failed.");
            if(false == bNext){break;}
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputOrderby),"Fill Order by error...");
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputGroupBy),"Fill Group by error...");
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue error");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteOrderby(),"ExecuteOrderby error...");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteGroupby(),"ExecuteGroupby error...");
            CHECK_RET_FILL_BREAK(m_tCacheTable.GetAggValue(m_pDataAddr,pCacheRow),"GetAggValue error");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteSQL(),"ExecuteSQL failed.");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteHaving(),"ExecuteHaving failed.");//����havingֵ
            CHECK_RET_FILL_BREAK(m_tCacheTable.SetAggValue(m_pDataAddr,pCacheRow),"SetAggValue failed.");
        }
        if(IsNeedReadLock())
        {
            m_pTable->tTableMutex.UnLock(m_pTable->bReadLock);//�ӱ���
        }
		
		
		if(m_pMdbSqlParser->m_listOutputGroupBy.iItemNum != 0)
			m_tCacheTable.DestroyHashTable();
    	//m_tCacheTable.FinishInsert();
        m_tCacheTable.FinishInsert(m_pMdbSqlParser->m_listOutputLimit);
		
        m_iNextType = NEXT_CACHE;
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * ��������	:  FillCollist
    * ��������	:  �����ֵ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::FillCollist()
    {
        TADD_FUNC("Start.m_iNextType=[%d].",m_iNextType);
        int iRet = 0;
        CHECK_RET_FILL(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue error");
        CHECK_RET_FILL(m_pMdbSqlParser->ExecuteSQL(), "%s", m_pMdbSqlParser->m_tError.GetErrMsg());
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * ��������	:  CacheNext
    * ��������	:  ����next
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbExecuteEngine::CacheNext()
    {
         if(m_tCacheTable.Next(m_pDataAddr))
         {
            return true;
         }
         else
         {
            return false;
         }
    }


    /******************************************************************************
    * ��������	:  NextWhere
    * ��������	:  ����where�����ļ�¼
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::NextWhere(bool & bResult)
    {
        int iRet = 0;

		if(m_bScanAll)
		{
			CHECK_RET(NextWhereByPage(bResult),"NextWhereByPage Failed");
		}
		else
		{
			CHECK_RET(NextWhereByIndex(bResult),"NextWhereByIndex Failed");
		}
       
        return iRet;
    }

	
    int TMdbExecuteEngine::NextWhereByPage(bool & bResult)
    {
		TADD_FUNC("Start.");
		int iRet = 0;
		if(m_MdbTableWalker.m_iScanPageType == TYPE_SCAN_PAGE_END)
		{
			bResult = false;
			return iRet;
		}

		//����ɾ��������ҳ����fullǨ�Ƶ�free�������ȱ���free		
		//����free��
		if(m_MdbTableWalker.m_iScanPageType == TYPE_SCAN_PAGE_FREE)
		{			
			if(NULL == m_MdbTableWalker.m_pCurPage)
			{
				//��ҪԤ�ȱ���iFullPageID��iFreePageID����Ϊɾ�������л�仯
				m_MdbTableWalker.m_iFirstFullPageID = m_pTable->iFullPageID;
				m_MdbTableWalker.m_iFirstFreePageID = m_pTable->iFreePageID;
				
	        	m_MdbTableWalker.WalkByPage(m_MdbTableWalker.m_iFirstFreePageID);
			}
	        while(m_MdbTableWalker.NextByPage())
	        {
	            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
	            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
	            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
	            m_iPagePos      = m_MdbTableWalker.GetPagePos();

				CHECK_RET(CheckWhere(bResult),"CheckWhere failed.");
				if(bResult)
				{
					m_MdbTableWalker.m_iAffect ++;
				   TADD_FLOW("where condition is satisfy.");				   
				   return iRet;
				   
				}
				else
				{
				  TADD_FLOW("where condition is not satisfy.");
				  continue;
				}
	        }
			//TADD_NORMAL("Scan Free End,Swicth to Full,Affect = %d\n",m_MdbTableWalker.m_iAffect);
			m_MdbTableWalker.m_iScanPageType = TYPE_SCAN_PAGE_FULL;
			m_MdbTableWalker.m_pCurPage = NULL;
			m_MdbTableWalker.m_pStartPage = NULL;
			m_MdbTableWalker.m_pNextPage = NULL;
			bResult = false;
		}

		//����full ��
		if(m_MdbTableWalker.m_iScanPageType == TYPE_SCAN_PAGE_FULL)
		{
			if(NULL == m_MdbTableWalker.m_pCurPage)
			{
	        	m_MdbTableWalker.WalkByPage(m_MdbTableWalker.m_iFirstFullPageID);
			}
			while(m_MdbTableWalker.NextByPage())
	        {
	            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
	            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
	            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
	            m_iPagePos      = m_MdbTableWalker.GetPagePos();

				CHECK_RET(CheckWhere(bResult),"CheckWhere failed.");
				if(bResult)
				{
					m_MdbTableWalker.m_iAffect ++;
				   TADD_FLOW("where condition is satisfy.");				   
				   return iRet;
				   
				}
				else
				{
				  TADD_FLOW("where condition is not satisfy.");
				  continue;
				}
	        }
			//TADD_NORMAL("Scan Full End,Swicth to End,Affect = %d\n",m_MdbTableWalker.m_iAffect);
			m_MdbTableWalker.m_iScanPageType = TYPE_SCAN_PAGE_END;
			m_MdbTableWalker.m_pCurPage = NULL;
			m_MdbTableWalker.m_pStartPage = NULL;
			bResult = false;
		}	
        
		return iRet;
	}

	
    int TMdbExecuteEngine::NextWhereByIndex(bool & bResult)
    {
        TADD_FUNC("Start.");
		int iRet = 0;
		ST_TABLE_INDEX_INFO * pTableIndex = NULL;
		long long   llValue	= -1;
		while(1)
		{
		   if(m_MdbTableWalker.Next() == false)
		   {
			   //��ĳһ������������ѡ����һ������
			   TADD_FUNC("Finish walk one index.");
			   GetNextIndex(pTableIndex,llValue);
			   if(NULL == pTableIndex)
			   {
				   //����ȫ���������ˡ�����false;
				   TADD_FLOW("All indexs have been walked.");
				   bResult = false;
				   break;
			   }

				m_MdbTableWalker.WalkByIndex(pTableIndex, llValue);//����������			   
			   continue;
		   }
		   else
		   {
			   //��ȡ��һ����¼
			   m_pDataAddr	   = m_MdbTableWalker.GetDataAddr();
			   m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
			   m_pPageAddr	   = m_MdbTableWalker.GetPageAddr();
			   m_iPagePos	   = m_MdbTableWalker.GetPagePos();
			   TADD_FLOW("Get one record:DataAddr[%p],CurRowData[%d|%d],PageAddr[%p],PagePos[%d].",
						   m_pDataAddr,m_tCurRowIDData.GetPageID(),m_tCurRowIDData.GetDataOffset(),
						   m_pPageAddr,m_iPagePos);
			   if(IsDataPosBefore())
			   {
				   //����������λ��
				   TADD_FLOW("Data PosBefore");
				   continue;
			   }
			   CHECK_RET(CheckWhere(bResult),"CheckWhere failed.");
			   if(bResult)
			   {
				   //where ��������
				   TADD_FLOW("where condition is satisfy.");
				   break;
			   }
			   else
			   {
				   //where ����������
				  TADD_FLOW("where condition is not satisfy.");
				  continue;
			   }
		   }
		}
		return iRet;
	}
	

    /******************************************************************************
    * ��������	:  ClearLastExecute
    * ��������	:  ������һ�ε�ִ�н��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TMdbExecuteEngine::ClearLastExecute()
    {
        m_iCurIndex    = -1;
        m_bScanAll     = false;
        m_llScanAllPos = -1;
        m_iRowsAffected= 0;
        m_iIsStop      = 0;
        m_iNextType    = NEXT_NORMAL;
        m_iMoniNext    = 0;
        m_tCurRowIDData.Clear();
        m_MdbTableWalker.StopWalk();//������һ�α���
        m_tError.FillErrMsg(0,"");//���������Ϣ
        if(m_pMdbSqlParser != NULL){m_pMdbSqlParser->ClearLastExecute();}
        m_tCacheTable.Clear();
        m_tLimitCtrl.Clear();
    }
    /******************************************************************************
    * ��������	:  SetRollbackBlock
    * ��������	:  ���ûع���
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::SetRollbackBlock(TMdbRollback * pRollback,int iRBUnitPos)
    {
        m_pRollback = pRollback;
        m_iRBUnitPos = iRBUnitPos;
        return 0;
    }

    /******************************************************************************
    * ��������	:  PushRollbackData
    * ��������	:  �Ѽ���Ҫ�ع�������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::PushRollbackData(const char *pDataAddr,const char * pExtraDataAddr)
    {
        int iRet = 0;
        if(NULL != m_pRollback && m_pTable->bRollBack)
        {
            //ֻ���������е�ʱ�򣬲���Ҫ�ɼ��ع�����
            iRet = m_pRollback->PushData(m_iRBUnitPos,pDataAddr,m_iRowsAffected,pExtraDataAddr,&m_tRowCtrl);
        }
        return iRet;
    }



    /******************************************************************************
    * ��������	:  IsPKExist
    * ��������	:  ��������Ƿ��Ѵ���
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::IsPKExist()
    {
        TADD_FUNC("Start.");
        int iRet= 0;
        long long llValue = 0;
        TADD_FLOW("Check PK index[%s].",m_pMdbSqlParser->m_stSqlStruct.stIndexForVerifyPK.pstTableIndex->pIndexInfo->sName);
        CalcMemValueHash(m_pMdbSqlParser->m_stSqlStruct.stIndexForVerifyPK,llValue);//��������ֵ
        ST_INDEX_VALUE & stIndexForVerifyPK = m_pMdbSqlParser->m_stSqlStruct.stIndexForVerifyPK;
        m_MdbTableWalker.WalkByIndex(stIndexForVerifyPK.pstTableIndex, llValue);
        while(m_MdbTableWalker.Next())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();

            CHECK_RET_FILL(FillSqlParserValue(m_pMdbSqlParser->m_listInputPriKey),"FillSqlParserValue error.");
            if(TMdbMemValue::CompareMemValueList(m_pMdbSqlParser->m_listInputPriKey,
                                                 m_pMdbSqlParser->m_listOutputPriKey) == 0)
            {
                //������ͻ
                char sTemp[1024] = {0};
                int i = 0 ;
                for(i = 0; i<m_pMdbSqlParser->m_listOutputPriKey.iItemNum; ++i)
                {
                    ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listOutputPriKey.pValueArray[i];
                    if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                    {
                        sprintf(sTemp+strlen(sTemp),"|%s",pstMemValue->sValue);
                    }
                    else if(MemValueHasProperty(pstMemValue,MEM_Int))
                    {
                        sprintf(sTemp+strlen(sTemp),"|%lld",pstMemValue->lValue);
                    }
                }
                CHECK_RET_FILL(ERR_TAB_PK_CONFLICT,"primary key conflict...value[%s].",sTemp);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  IsDataPosBefore
    * ��������	:  �ü�¼�Ƿ񱻶�λ��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbExecuteEngine::IsDataPosBefore()
    {
        TADD_FUNC("Start.");
        bool bRet = false;
        int i = 0;
        long long lPosValue = 0;
        long long lDataValue = 0;
        
        int * arrRowIndex = NULL;
        for(i = 0; i<m_iCurIndex; ++i)
        {
            if(0 == i)
            {
               arrRowIndex  =GetRowIndexArray(m_pDataAddr);
            }
            ST_INDEX_VALUE & stIndexValue = (*m_pVIndex)[i];
            CalcMemValueHash(stIndexValue,lPosValue);
            lDataValue = (arrRowIndex[stIndexValue.pstTableIndex->iIndexPos]);
            TADD_DETAIL("PosValue=[%lld],DataValue=[%lld]",lPosValue,lDataValue);
            if(lPosValue == lDataValue)
            {
                bRet = true;
                break;
            }//֮ǰ������������λ��
        }
        TADD_FUNC("Finish.bRet[%d]",bRet);
        return bRet;
    }

    /******************************************************************************
    * ��������	:  CheckRowDataStruct
    * ��������	:  У��ṹ������Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::CheckRowDataStruct(int* Column)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pTable);
        if(1 == m_pTable->iColumnCounts){return iRet;} //ֻ��һ��������
        int i = 0;
        TMdbColumn * pMdbColumn = NULL;
        for(i = 0;i < m_pTable->iColumnCounts - 1 ;++i)
        {//�޷�У�����һ���еĳ���
            int iOutLen = Column[i+1] - Column[i];
            pMdbColumn = &(m_pTable->tColumn[i]);
            int iColumnLen = pMdbColumn->iColumnLen;
            bool bResult = false;//У����
            switch(pMdbColumn->iDataType)
            {
                case DT_Int://long long
                    bResult = (iOutLen == iColumnLen);
                    break;
                case DT_Blob:
                case DT_Char:
                case DT_VarChar:
                    {
                        if(1 == iOutLen)
                        {//���ַ�
                            bResult = (iColumnLen == 2);
                        }
                        else
                        {
                            bResult = (iOutLen >= iColumnLen);
                        }
                    }
                    break;
                case DT_DateStamp:
                    {
                        if(DATE_TIME_SIZE == iColumnLen)
                        {//��ѹ��
                           bResult = (iOutLen >= iColumnLen);
                        }
                        else
                        {//ѹ��ʱ��
                            bResult = (iOutLen == iColumnLen);
                        }
                    }
                    break;
                default:
                    CHECK_RET(ERR_APP_INVALID_PARAM,"Unknow dataType[%d]",pMdbColumn->iDataType);
                    break;
            }
            if(false == bResult)
            {//���Ȳ���ȷ
                TADD_WARNING("RowDataStruct Table[%s],Column[%s] ColumnLen=[%d] but OutLen=[%d].", 
                    m_pTable->sTableName,pMdbColumn->sName,iColumnLen,iOutLen);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetOneRowData
    * ��������	:  ��ȡһ��������Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::GetOneRowData(void *pStruct,int* Column)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ_FILL(m_pDataAddr);
        if(false == m_pMdbSqlParser->m_bGetValueChecked)
        {//���GetValue�ӿڣ�ֻ���һ��
            CheckRowDataStruct(Column);
            m_pMdbSqlParser->m_bGetValueChecked = true;
        }
        char* pStructAddr = (char*)pStruct;
        TMdbColumn * pMdbColumn = NULL;
        int i = 0;
        for(i = 0; i<m_pTable->iColumnCounts; ++i)
        {
            pMdbColumn = &(m_pTable->tColumn[i]);
            switch(pMdbColumn->iDataType)
            {
            case DT_Int:  //Integer
            {
                memcpy(pStructAddr+Column[i],&m_pDataAddr[pMdbColumn->iOffSet],pMdbColumn->iColumnLen);
                TADD_FLOW("Column[%d] value=[%lld],col_pos=[%d]",i,(long long)*(long long *)(pStructAddr+Column[i]),Column[i]);
                break;
            }
            case DT_Char://Char
            {
                memcpy(pStructAddr+Column[i],&m_pDataAddr[pMdbColumn->iOffSet],pMdbColumn->iColumnLen);
                TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                break;
            }
            case DT_Blob:
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULLֵ
		            *(pStructAddr+Column[i]) = 0;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
                CHECK_RET_FILL_CODE(m_tVarcharCtrl.GetVarcharValue(pStructAddr+Column[i], m_pDataAddr+pMdbColumn->iOffSet),
                               ERR_SQL_GET_MEMORY_VALUE_ERROR,"GetVarcharValue ERROR");
                TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                //Base64����
                std::string encoded = (pStructAddr+Column[i]);
                std::string decoded = Base::base64_decode(encoded);
                memset(pStructAddr+Column[i],0,strlen(pStructAddr+Column[i])); //���ԭ������
                memcpy(pStructAddr+Column[i],decoded.c_str(),decoded.length());
            }
            break;
            case DT_VarChar:  //VarChar
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULLֵ
		        	*(pStructAddr+Column[i]) = 0;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
                CHECK_RET_FILL_CODE(m_tVarcharCtrl.GetVarcharValue(pStructAddr+Column[i], m_pDataAddr+pMdbColumn->iOffSet),
                               ERR_SQL_GET_MEMORY_VALUE_ERROR,"GetVarcharValue ERROR");
                TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                break;
            }
            case DT_DateStamp: //ʱ����string����
            {
                memcpy(pStructAddr+Column[i],&m_pDataAddr[pMdbColumn->iOffSet],pMdbColumn->iColumnLen);
                if(sizeof(int) == pMdbColumn->iColumnLen)
                {
                    TADD_FLOW("Column[%d] value=[%d],col_pos=[%d]",i,(int)*(int *)(pStructAddr+Column[i]),Column[i]);
                }
                else if(sizeof(long long) == pMdbColumn->iColumnLen)
                {
                    TADD_FLOW("Column[%d] value=[%lld],col_pos=[%d]",i,(long long)*(long long *)(pStructAddr+Column[i]),Column[i]);
                }
                else 
                {
                    TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                }
                break;
            }
            default:
                CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,"column[%s]DataType=%d invalid.", pMdbColumn->sName,pMdbColumn->iDataType);
                break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbExecuteEngine::GetOneRowData(TMdbColumnAddr* pTColumnAddr)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ_FILL(m_pDataAddr);
        TMdbColumn * pMdbColumn = NULL;
        int i = 0;
        for(i = 0; i<m_pTable->iColumnCounts; ++i)
        {
            pMdbColumn = &(m_pTable->tColumn[i]);
            switch(pMdbColumn->iDataType)
            {
            case DT_Int:  //Integer            
            case DT_Char: //Char
            case DT_DateStamp://ʱ����string����
            {
				pTColumnAddr->m_ppColumnAddr[i] = &m_pDataAddr[pMdbColumn->iOffSet];
				pTColumnAddr->m_iDataLen[i] = pMdbColumn->iColumnLen;
                break;
            }
            case DT_VarChar:  //VarChar
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULLֵ
		        	pTColumnAddr->m_ppColumnAddr[i] = NULL;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
				int iWhichPos = 0;
				unsigned int iRowId = 0;
				m_tVarcharCtrl.GetStoragePos(m_pDataAddr+pMdbColumn->iOffSet,iWhichPos,iRowId);
				TMdbRowID tRowId;
			    tRowId.SetRowId(iRowId);
			    pTColumnAddr->m_ppColumnAddr[i] = m_tVarcharCtrl.GetAddressRowId(&tRowId);	
				pTColumnAddr->m_iDataLen[i] = m_tVarcharCtrl.GetValueSize(iWhichPos);
                break;
            }
			case DT_Blob:
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULLֵ
		        	pTColumnAddr->m_ppColumnAddr[i] = NULL;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
                int iWhichPos = 0;
				unsigned int iRowId = 0;
				int iEnCodeLen = 0;
				m_tVarcharCtrl.GetStoragePos(m_pDataAddr+pMdbColumn->iOffSet,iWhichPos,iRowId);
				TMdbRowID tRowId;
			    tRowId.SetRowId(iRowId);
			    std::string encoded = m_tVarcharCtrl.GetAddressRowId(&tRowId);
				std::string decoded = Base::base64_decode(encoded);
				iEnCodeLen =  decoded.length();
				
				pTColumnAddr->m_iDataLen[i] = iEnCodeLen;
				SAFE_DELETE(pTColumnAddr->m_ppBlob[i]);
				pTColumnAddr->m_ppBlob[i] = new char[iEnCodeLen];
				memcpy(pTColumnAddr->m_ppBlob[i],decoded.c_str(),iEnCodeLen);
				
				pTColumnAddr->m_ppColumnAddr[i] = pTColumnAddr->m_ppBlob[i];

				break;
            }
           
            default:
                CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,"column[%s]DataType=%d invalid.", pMdbColumn->sName,pMdbColumn->iDataType);
                break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	

    long long TMdbExecuteEngine::GetRowTimeStamp()
    {
        TADD_FUNC("Start.");
        int  iRet = 0;
        long long iTimeStamp = 0;
        iRet = m_tRowCtrl.GetTimeStamp(m_pDataAddr, m_pTable->m_iTimeStampOffset, iTimeStamp);
        if(iRet != 0)
        {
            iTimeStamp = 0;
        }
        return iTimeStamp;
    }


    /******************************************************************************
    * ��������	:  GetRowIndexArray
    * ��������	:  ��ȡrowindex����
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int * TMdbExecuteEngine::GetRowIndexArray(char * pDataAddr)
    {
        TADD_FUNC("Start");
        if(NULL == m_pTable)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"table is null");
            return NULL;
        }
        if(NULL == m_aRowIndexPos)
        {
            m_aRowIndexPos = new int[m_pTable->iIndexCounts];
        }
        if(NULL == m_aRowIndexPos){return NULL;}
        if(NULL != pDataAddr)
        {//��������ֵ,��������ֵ
            int i = 0;
            int iError = 0;
            for(i = 0;i < m_pTable->iIndexCounts;++i)
            { 
                m_aRowIndexPos[i] = -1;
                m_aRowIndexPos[i] = m_mdbIndexCtrl.CalcIndexValue(m_tRowCtrl, pDataAddr, &(m_pTable->tIndex[i]), iError);
                if(0 != iError)
                {
                    TADD_ERROR(iError,"CalcIndexValue(iError=%d) failed.",iError);
                    return NULL;
                }
            }
        }
        return m_aRowIndexPos;
    }
    

    /******************************************************************************
    * ��������	:  GetUpdateDiff
    * ��������	:  ��ȡ��������ֵ,ֻ��ȡ��ֵ�͵�����
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::GetUpdateDiff()
    {
        TADD_FUNC("Start");
        int iRet = 0;
        CHECK_OBJ_FILL(m_pUpdateBlock);
        int i = 0;
        for(i = 0; i<m_pMdbSqlParser->m_listOutputCollist.iItemNum; i++)
        {
            ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listOutputCollist.pValueArray[i];
            int iOffSet = pstMemValue->pColumnToSet->iOffSet;
            //if(DT_Int == pstMemValue->pColumnToSet->iDataType)
            if(pstMemValue->pColumnToSet->bIncrementalUpdate())
            {//������
                    long long * pDiffInt = (long long*)&m_pUpdateBlock[iOffSet];
                    * pDiffInt = (*(long long*)&m_pDataAddr[iOffSet]) - pstMemValue->lValue;//��ȡ��ֵ
                    TADD_DETAIL("pDiffInt=[%lld]",*pDiffInt);
            }
        }
        //��ȡ����ֵ
        for(i = 0; i<m_pMdbSqlParser->m_listInputPriKey.iItemNum; i++)
        {
            ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listInputPriKey.pValueArray[i];
            int iOffSet = pstMemValue->pColumnToSet->iOffSet;
            if(DT_Int == pstMemValue->pColumnToSet->iDataType)
            {//��ֵ��
                    long long * pPKInt = (long long*)&m_pUpdateBlock[iOffSet];
                    *pPKInt = pstMemValue->lValue;
                    TADD_DETAIL("pPKInt=[%lld]",*pPKInt);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbExecuteEngine::SetRowDataTimeStamp(char* pAddr, int iOffset,long long iTimeStamp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        long long* piTimeStamp = (long long*)&pAddr[iOffset];

        if(iTimeStamp == 0)
        {
            *piTimeStamp = TMdbDateTime::StringToTime( m_pDsn->sCurTime);
        }
        else
        {
            *piTimeStamp = iTimeStamp;
        }

        return iRet;
    }

    int TMdbExecuteEngine::UpdateRowDataTimeStamp(char* const & pAddr,int iOffset, long long iTimeStamp )
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        if(iTimeStamp == 0)
        {
            iRet = m_tRowCtrl.SetTimeStamp(pAddr,iOffset,(long long) TMdbDateTime::StringToTime( m_pDsn->sCurTime));
        }
        else
        {
            iRet = m_tRowCtrl.SetTimeStamp(pAddr, iOffset,iTimeStamp);
        }

        return iRet;
    }

    /******************************************************************************
    * ��������	:  ReBuildTableFromPage
    * ��������	:  ���ڴ�ҳ���¹�����,����
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ReBuildTableFromPage(const char * sDSN,TMdbTable * pMdbTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        ClearLastExecute();
        CHECK_OBJ(pMdbTable);
        CHECK_OBJ(sDSN);
        m_pTable = pMdbTable;
        TMdbShmDSN * pShmDSN = TMdbShmMgr::GetShmDSN(sDSN);
        CHECK_OBJ(pShmDSN);
        m_pDsn = pShmDSN->GetInfo();
        m_mdbIndexCtrl.AttachTable(pShmDSN,pMdbTable);
        CHECK_RET(m_MdbTableWalker.AttachTable(pShmDSN,m_pTable),"AttachTable failed.");
        CHECK_RET_FILL_CODE(m_tRowCtrl.Init(m_pDsn->sName,m_pTable->sTableName),ERR_OS_ATTACH_SHM,"m_tRowCtrl.AttachTable failed.");//��¼����
        //����full ��
        m_MdbTableWalker.WalkByPage(m_pTable->iFullPageID);
        while(m_MdbTableWalker.NextByPage())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();
            TADD_DETAIL("page_id=[%d],offset=[%d],rowid=[%ud].",m_pTable->iFullPageID,m_iPagePos,m_tCurRowIDData.m_iRowID);
            CHECK_RET(ChangeInsertIndex(m_pDataAddr,m_tCurRowIDData),"ChangeInsertIndex failed.");//��������
            m_pTable->iCounts++;
        }
        //����free ��
        m_MdbTableWalker.WalkByPage(m_pTable->iFreePageID);
        while(m_MdbTableWalker.NextByPage())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();
            TADD_DETAIL("page_id=[%d],offset=[%d],rowid=[%d].",m_tCurRowIDData.GetPageID(),m_tCurRowIDData.GetDataOffset(),m_tCurRowIDData.m_iRowID);
            CHECK_RET(ChangeInsertIndex(m_pDataAddr,m_tCurRowIDData),"ChangeInsertIndex failed.");//��������
            m_pTable->iCounts ++;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbExecuteEngine::CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_sTempValue[0] = 0;
        if(HT_CMP ==  stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
            //�������
            int i = 0;
            for(i = 0; i < MAX_INDEX_COLUMN_COUNTS; ++i)
            {
                if(stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i] < 0)
                {
                    break;
                }
                CHECK_OBJ(stIndexValue.pExprArr[i]);
                ST_MEM_VALUE * pstMemValue = stIndexValue.pExprArr[i]->pExprValue;
                CHECK_OBJ(pstMemValue);
                if(pstMemValue->IsNull())
                {
                    sprintf(m_sTempValue, "%s;0",m_sTempValue);
                }
                else if(MemValueHasProperty(pstMemValue,MEM_Int))
                {
                    sprintf(m_sTempValue, "%s;%lld", m_sTempValue, pstMemValue->lValue);
                }
                else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                {
                    sprintf(m_sTempValue, "%s;%s", m_sTempValue, pstMemValue->sValue);
                }
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        }
        else
        {
            //������
            if(stIndexValue.pExprArr[0]->pExprValue->IsNull())
            {
                llValue = 0;
            }
            else  if(MemValueHasAnyProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Str|MEM_Date))
            {
                llValue = TMdbNtcStrFunc::StrToHash(stIndexValue.pExprArr[0]->pExprValue->sValue);
            }
            else if(MemValueHasProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Int))
            {
                llValue = stIndexValue.pExprArr[0]->pExprValue->lValue;
            }
        }
        llValue = llValue<0? -llValue:llValue;
        if(stIndexValue.pstTableIndex->pIndexInfo->m_iAlgoType == INDEX_HASH)
        {
            //����ɢ��
            llValue = llValue % m_pTable->iRecordCounts; 
        }
        TADD_FUNC("Finish.llValue = %lld.m_sTempValue=[%s].",llValue,m_sTempValue);
        return iRet;
    }
//}
