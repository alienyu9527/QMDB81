/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbQueue.h
*@Description： 对oralce同步区和主备同步区进行封装，提供统一操作接口
*@Author:		cao.peng
*@Date：	    2012.11
*@History:
******************************************************************************************/
#ifndef _MDB__QUEUE_H_
#define _MDB__QUEUE_H_

#include "Helper/TThreadLog.h" 
#include "Control/mdbMgrShm.h"
#include "Control/mdbProcCtrl.h"

//namespace QuickMDB{

#define ERROR_TIMES 10
#define  T_SUCCESS                0    
#define  T_BIG_PushPos         1    
#define  T_BIG_TailPos           2   
#define  T_BIG_ENDERROR      3
#define  T_BIG_BEGINERROR   4
#define  T_EMPTY                     5
#define  T_LENGTH_ERROR       6
#define  T_SQLTYPE_ERROR      7
#define  T_SOURCEID_ERROR    8

#define T_UPDATE                          2
#define T_DELETE                          3
#define T_INSERT                          4


class TMdbQueue
{
public:
	TMdbQueue();
	~TMdbQueue();
	int Init(TMdbMemQueue * pMemQueue,TMdbDSN * pDsn,const bool bWriteErrorData=false);
	
	/******************************************************************************
	* 函数名称	:  Push
	* 函数描述	:  设置DSN名称
	* 输入		:  sData push的数据，iLen push数据的长度
	* 输出		:  无
	* 返回值	:  !0 - 失败,0 - 成功
	* 作者		:  cao.peng
	*******************************************************************************/
	bool Push(char * const sData,const int iLen);

	/******************************************************************************
	* 函数名称	:  Pop
	* 函数描述	:  获取队列中一条数据
	* 输入		:  无
	* 输出		:  sData 存储pop出的数据，iBufLen 存储pop数据的长度，iLen pop出数据的长度
	* 返回值	:  !0 - 失败,0 - 成功
	* 作者		:  cao.peng
	*******************************************************************************/	
	int Pop();

    int PopRepData();

	/******************************************************************************
	* 函数名称	:  GetSourceId
	* 函数描述	:  获取pop的数据的数据来源
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  数据源
	* 作者		:  cao.peng
	*******************************************************************************/
	//int GetSourceId();

	/******************************************************************************
	* 函数名称	:  GetSqlType
	* 函数描述	:  获取sql类型
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  sql类型
	* 作者		:  cao.peng
	*******************************************************************************/
	int GetSqlType();

	int GetRecordLen();
	char* GetData();

	void SetParameter();
       void SetCheckDataFlag(bool bCheck){m_bCheckData = bCheck;}//是否校验数据
private:
	/******************************************************************************
	* 函数名称	:  CheckDataIsValid
	* 函数描述	:  判断队列中的某条数据是否有效
	* 输入		:  pCurAddr 检测共享内存地址
	* 输出		:  iLen 数据长度
	* 返回值	:  true 数据有效；false 数据无效
	* 作者		:  cao.peng
	*******************************************************************************/
	int CheckDataIsValid();

    int CheckRepDataIsValid();
		
	/******************************************************************************
	* 函数名称	:  CheckRecord
	* 函数描述	:  判断pop出的数据是否合法，主要检查数据的长度以及是否以##结束
	* 输入		:  pszRecord 检测记录,iLen 记录长度
	* 输出		:  无
	* 返回值	:  true 记录合法；false 记录无效
	* 作者		:  cao.peng
	*******************************************************************************/
	bool CheckRecord(char *pszRecord,const int iLen);

	/******************************************************************************
	* 函数名称	:  PrintInvalidData
	* 函数描述	:  打印错误记录信息
	* 输入		:  pCurAddr 数据地址，iLen无效数据长度
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  cao.peng
	*******************************************************************************/
	void PrintInvalidData(char *pCurAddr,const int iLen);
	int GetPosOfNext();//获取下一条记录位置
	bool WriteInvalidData(const int iInvalidLen);//记录无效记录
private:
	TMdbDSN      * m_pDsn;
	TMdbMemQueue * m_pQueueShm;    //数据存储地址 
	char* m_pszRecord;
	char * m_pCurAddr;
	int m_iSQLType;                //sql类型
	int m_iSyncType;               //数据来源
	int m_iRecordLen;            //数据长度
	int m_iErrTry;                 //pop数据时，检查错误次数

	int m_iPushPos;
	int m_iPopPos;
	int m_iTailPos;
	bool m_bCheckData;//是否校验数据
	char m_sFileName[MAX_PATH_NAME_LEN];
	char *m_pszErrorRecord;
	FILE* m_pFile;
	bool m_bWriteErrorData;
};

class TMdbOnlineRepQueue
{
public:
	TMdbOnlineRepQueue();
	~TMdbOnlineRepQueue();
	int Init(TMdbOnlineRepMemQueue * pOnlineRepMemQueue,TMdbDSN * pDsn,const bool bWriteErrorData=false);
	
	/******************************************************************************
	* 函数名称	:  Push
	* 函数描述	:  设置DSN名称
	* 输入		:  sData push的数据，iLen push数据的长度
	* 输出		:  无
	* 返回值	:  !0 - 失败,0 - 成功
	* 作者		:  cao.peng
	*******************************************************************************/
	bool Push(char * const sData,const int iLen);

	/******************************************************************************
	* 函数名称	:  Pop
	* 函数描述	:  获取队列中一条数据
	* 输入		:  无
	* 输出		:  sData 存储pop出的数据，iBufLen 存储pop数据的长度，iLen pop出数据的长度
	* 返回值	:  !0 - 失败,0 - 成功
	* 作者		:  cao.peng
	*******************************************************************************/	
	int Pop();

	int RollbackPopPos();

	int GetPosOfNext();

	int GetRecordLen();
	char* GetData();
	int GetUsedPercentage();
	int CheckRepDataIsValid();
	bool WriteInvalidData(const int iInvalidLen);
private:
	

	/******************************************************************************
	* 函数名称	:  PrintInvalidData
	* 函数描述	:  打印错误记录信息
	* 输入		:  pCurAddr 数据地址，iLen无效数据长度
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  cao.peng
	*******************************************************************************/
	void PrintInvalidData(char *pCurAddr,const int iLen);
private:
	TMdbDSN      * m_pDsn;
	TMdbOnlineRepMemQueue * m_pOnlineRepQueueShm;    //链路同步数据存储地址 
	char* m_pszRecord;
	char * m_pCurAddr;
	int m_iSQLType;                //sql类型
	int m_iSyncType;               //数据来源
	int m_iRecordLen;            //数据长度
	int m_iErrTry;                 //pop数据时，检查错误次数

	int m_iPushPos;
	int m_iPopPos;
	int m_iCleanPos;
	int m_iStartPos;
	int m_iTailPos;
	bool m_bCheckData;//是否校验数据
	char m_sFileName[MAX_PATH_NAME_LEN];
	char *m_pszErrorRecord;
	FILE* m_pFile;
	bool m_bWriteErrorData;
};

//}
#endif
