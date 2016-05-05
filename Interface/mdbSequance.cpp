/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbSequance.h
*@Description�� �������miniDB�����еĿ���
*@Author:		li.shugang
*@Date��	    2009��06��16��
*@History:
******************************************************************************************/
#include "Interface/mdbSequance.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbConfig.h"
#include "Interface/mdbQuery.h"

//namespace QuickMDB{

/******************************************************************************
* ��������	:  TMdbSequence
* ��������	: ���й���
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TMdbSequence::TMdbSequence()
{
    m_pSeq    = NULL;
    m_pConfig = NULL;
}
TMdbSequence::~TMdbSequence()
{
}
/******************************************************************************
* ��������	:  SetConfig()
* ��������	:  ����DSN����������,��λ����������е�����
* ����		:  pszDSN, ���ݿ�ʵ��������
* ����		:  pszSeqName, ���е�����
* ���		:  ��
* ����ֵ	:  �ɹ�����0��ʧ���򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
int TMdbSequence::SetConfig(const char* pszDSN, const char *pszSeqName)
{
    int iRet = 0;
    CHECK_OBJ(pszDSN);
    m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
    CHECK_OBJ(m_pConfig);
    CHECK_RET(SetConfig(pszDSN,pszSeqName,m_pConfig),"SetConfig failed.");
    return iRet;
}
/******************************************************************************
* ��������	:  SetConfig()
* ��������	:  ����DSN����������,��λ����������е�����
* ����		:  pszDSN, ���ݿ�ʵ��������
* ����		:  pszSeqName, ���е�����
* ���		:  ��
* ����ֵ	:  �ɹ�����0��ʧ���򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
int TMdbSequence::SetConfig(const char* pszDSN, const char *pszSeqName,TMdbConfig *pConfig)
{
    int iRet = 0;
    CHECK_OBJ(pszDSN);
    CHECK_OBJ(pConfig);
    TMdbShmDSN* pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);//�����Ϲ����ڴ�
    CHECK_OBJ(pShmDSN);
    CHECK_RET(pShmDSN->Attach(pszDSN, *pConfig)," Attach [%s] failed. Parameters are invalid.",pszDSN);
    m_pSeq =pShmDSN->GetMemSeqByName(pszSeqName);
    if(NULL == m_pSeq)
    {
        TADD_ERROR(ERR_DB_SEQUECE_NOT_EXIST,"Can't find Sequence=[%s].dsn=[%s]", pszSeqName,pszDSN);
        return ERR_DB_SEQUECE_NOT_EXIST;
    }
    return iRet;
}

/******************************************************************************
* ��������	:  GetNextIntVal()
* ��������	:  ��ȡ��һ������ֵ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ������һ������ֵ
* ����		:  li.shugang
*******************************************************************************/
MDB_INT64 TMdbSequence::GetNextIntVal()
{
    TADD_FUNC("Start.");
    if(m_pSeq == NULL)
    {
        TADD_ERROR(ERR_DB_SEQUECE_NOT_EXIST,"NULL m_pSeq");
        return ERR_DB_SEQUECE_NOT_EXIST;
    }
    m_pSeq->tMutex.Lock(true);
    MDB_INT64 iRet = m_pSeq->iCur;
    if(iRet+m_pSeq->iStep > m_pSeq->iEnd)
    {
        m_pSeq->iCur = m_pSeq->iStart;
    }
    else
    {
        m_pSeq->iCur += m_pSeq->iStep;
    }
    iRet = m_pSeq->iCur;
    m_pSeq->tMutex.UnLock(true);
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  GetStep()
* ��������	:  ��ȡ����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ���ز���
* ����		:  li.shugang
*******************************************************************************/
MDB_INT64 TMdbSequence::GetStep()
{
    if(m_pSeq == NULL)
    {
        TADD_ERROR(ERR_DB_SEQUECE_NOT_EXIST,"NULL m_pSeq");
        return ERR_DB_SEQUECE_NOT_EXIST;
    }
    return m_pSeq->iStep;
}

/******************************************************************************
* ��������	:  SetStep()
* ��������	:  ���ò���
* ����		:  iStep ����
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  cao.peng
*******************************************************************************/
MDB_INT64 TMdbSequence::SetStep(const int iStep)
{
    int iRet = 0;
    CHECK_OBJ(m_pSeq);
    CHECK_RET(m_pSeq->tMutex.Lock(true),"lock failed.");
    m_pSeq->iStep = iStep;
    CHECK_RET(m_pSeq->tMutex.UnLock(true),"unlock failed.");
    return iRet;
}

/******************************************************************************
* ��������	:  SetStartSeq()
* ��������	:  ���ÿ�ʼ����
* ����		:  iStartSeq ��ʼ����
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  cao.peng
*******************************************************************************/
MDB_INT64 TMdbSequence::SetStartSeq(const int iStartSeq)
{
    int iRet = 0;
    CHECK_OBJ(m_pSeq);
    CHECK_RET(m_pSeq->tMutex.Lock(true),"lock failed.");
    m_pSeq->iStart = iStartSeq;
    CHECK_RET(m_pSeq->tMutex.UnLock(true),"unlock failed.");
    return iRet;
}

/******************************************************************************
* ��������	:  SetEndSeq()
* ��������	:  ���ý�������
* ����		:  iEndSeq ��������
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  cao.peng
*******************************************************************************/
MDB_INT64 TMdbSequence::SetEndSeq(const int iEndSeq)
{
    int iRet = 0;
    CHECK_OBJ(m_pSeq);
    CHECK_RET(m_pSeq->tMutex.Lock(true),"lock failed.");
    m_pSeq->iEnd = iEndSeq;
    CHECK_RET(m_pSeq->tMutex.UnLock(true),"unlock failed.");
    return iRet;
}

/******************************************************************************
* ��������	:  SetCurSeq()
* ��������	:  ���õ�ǰ����ֵ
* ����		:  iCurSeq ����ֵ
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  cao.peng
*******************************************************************************/
MDB_INT64 TMdbSequence::SetCurSeq(const int iCurSeq)
{
    int iRet = 0;
    CHECK_OBJ(m_pSeq);
    CHECK_RET(m_pSeq->tMutex.Lock(true),"lock failed.");
    //�ж��޸ĵĵ�ǰֵ�Ƿ�Ϊ:��ʼֵ+N*������������澯
    if((iCurSeq-m_pSeq->iStart)%m_pSeq->iStep != 0)
    {
        TADD_WARNING("Suggested sequence[%s] current value[%d] is set to start sequence plus N times the step value.",\
            m_pSeq->sSeqName,iCurSeq);
    }
    if(iCurSeq > m_pSeq->iEnd)
    {
        m_pSeq->iCur = m_pSeq->iEnd;
    }
    else
    {
        m_pSeq->iCur = iCurSeq;
    }
    CHECK_RET(m_pSeq->tMutex.UnLock(true),"unlock failed.");
    return iRet;
}

/******************************************************************************
* ��������	:  GetCurSeq()
* ��������	:  ��ȡ��ǰseq
* ����		: 
* ���		:  ��
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
MDB_INT64 TMdbSequence::GetCurrVal()
{
    return m_pSeq->iCur;
}

/******************************************************************************
* ��������	:  Clear
* ��������	:  ��ʼ����ǰ����(����clear��ǰ����ʹ�õ�����)
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 �ɹ�����0ʧ��
* ����		:  cao.peng
*******************************************************************************/
MDB_INT64 TMdbSequence::Clear()
{
    int iRet = 0;
    CHECK_OBJ(m_pSeq);
    CHECK_RET(m_pSeq->tMutex.Lock(true),"lock failed.");
    m_pSeq->Clear();
    CHECK_RET(m_pSeq->tMutex.UnLock(true),"unlock failed.");
    return iRet;
}
//}
