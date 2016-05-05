/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbSequance.h
*@Description： 负责管理miniDB的序列的控制
*@Author:		li.shugang
*@Date：	    2009年06月16日
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
* 函数名称	:  TMdbSequence
* 函数描述	: 序列构造
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
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
* 函数名称	:  SetConfig()
* 函数描述	:  设置DSN和序列名称,定位到具体的序列点上面
* 输入		:  pszDSN, 数据库实例的名称
* 输入		:  pszSeqName, 序列的名称
* 输出		:  无
* 返回值	:  成功返回0，失败则返回-1
* 作者		:  li.shugang
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
* 函数名称	:  SetConfig()
* 函数描述	:  设置DSN和序列名称,定位到具体的序列点上面
* 输入		:  pszDSN, 数据库实例的名称
* 输入		:  pszSeqName, 序列的名称
* 输出		:  无
* 返回值	:  成功返回0，失败则返回-1
* 作者		:  li.shugang
*******************************************************************************/
int TMdbSequence::SetConfig(const char* pszDSN, const char *pszSeqName,TMdbConfig *pConfig)
{
    int iRet = 0;
    CHECK_OBJ(pszDSN);
    CHECK_OBJ(pConfig);
    TMdbShmDSN* pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);//连接上共享内存
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
* 函数名称	:  GetNextIntVal()
* 函数描述	:  获取下一个序列值
* 输入		:  无
* 输出		:  无
* 返回值	:  返回下一个序列值
* 作者		:  li.shugang
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
* 函数名称	:  GetStep()
* 函数描述	:  获取步长
* 输入		:  无
* 输出		:  无
* 返回值	:  返回步长
* 作者		:  li.shugang
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
* 函数名称	:  SetStep()
* 函数描述	:  设置步长
* 输入		:  iStep 步长
* 输出		:  无
* 返回值	:  0 成功；非0失败
* 作者		:  cao.peng
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
* 函数名称	:  SetStartSeq()
* 函数描述	:  设置开始序列
* 输入		:  iStartSeq 开始序列
* 输出		:  无
* 返回值	:  0 成功；非0失败
* 作者		:  cao.peng
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
* 函数名称	:  SetEndSeq()
* 函数描述	:  设置结束序列
* 输入		:  iEndSeq 结束序列
* 输出		:  无
* 返回值	:  0 成功；非0失败
* 作者		:  cao.peng
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
* 函数名称	:  SetCurSeq()
* 函数描述	:  设置当前序列值
* 输入		:  iCurSeq 序列值
* 输出		:  无
* 返回值	:  0 成功；非0失败
* 作者		:  cao.peng
*******************************************************************************/
MDB_INT64 TMdbSequence::SetCurSeq(const int iCurSeq)
{
    int iRet = 0;
    CHECK_OBJ(m_pSeq);
    CHECK_RET(m_pSeq->tMutex.Lock(true),"lock failed.");
    //判断修改的当前值是否为:起始值+N*步长，不是则告警
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
* 函数名称	:  GetCurSeq()
* 函数描述	:  获取当前seq
* 输入		: 
* 输出		:  无
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
MDB_INT64 TMdbSequence::GetCurrVal()
{
    return m_pSeq->iCur;
}

/******************************************************************************
* 函数名称	:  Clear
* 函数描述	:  初始化当前序列(不能clear当前正在使用的序列)
* 输入		:  无
* 输出		:  无
* 返回值	:  0 成功；非0失败
* 作者		:  cao.peng
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
