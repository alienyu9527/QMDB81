/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbCtrl.h
*@Description： 内存数据库的控制程序
*@Author:		li.shugang
*@Date：	    2008年11月25日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_CTROL_H__
#define __MINI_DATABASE_CTROL_H__

#include "Helper/mdbConfig.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbProcCtrl.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Replication/mdbRepCtrl.h"
#include <vector>

//namespace QuickMDB{

class TMdbCtrl
{
public:
    TMdbCtrl();
    ~TMdbCtrl();
public:
    int Init(const char* pszDsn);     //初始化
    int Create();   //创建某个内存数据库
    int Destroy(const char* pszDSN, bool bForceFlag = false);  //销毁某个内存数据库
    int Start();   //启动某个内存数据库
    int Stop(const char*pszDSN);    //停止某个内存数据库
    bool IsMdbCreate();//是否有mdb进程存在
    bool IsStop(); //数据库是否需要停止
    void SetLoadFromDisk(bool bFlag){m_bLoadFromDisk = bFlag;}//设定从磁盘加载
private:
    int LockFile();  //锁住启动标识文件:-1-文件不存在;0-加锁失败;1-加锁成功
    int UnLockFile();
    int GetStartMethod();//获得正确的启动方式
    int CreateSysMem(); //创建共享内存块
    int CreateTableSpace();//创建表空间
    int CreateTable();//创建内存表
    int CreateVarChar();//创建varchar管理区
    int LoadData();//上载数据
    int LoadFromOra();  //从Oracle上载全量数据
    int LoadFromStandbyHost();//从备机上载数据
    int LoadFromShardBackupHost(); // 分片备份上载
    int LoadSysData();   //初始化系统表
    int LoadDBAUser(TMdbDatabase* pMdb);//上载dba_user
    int LoadDBADual(TMdbDatabase* pMdb);//上载dual表
    int LoadSequence();
    bool CheckVersion(TMdbDSN *pTMdbDSN);//校验内核版本和发布版本，来确定是否要重新创建共享内存
    int CheckSystem();//校验配置项
    int GetCSMethod(int iAgentPort);
    int GetProcToStart(std::vector<std::string > & vProcToStart);//获取要启动的进程信息
    bool NeedRemoveFile();
    int LoadFromDisk();//从磁盘加载
	int CreateSBBufShm();

    /******************************************************************************
    * 函数名称	:  CheckTablespace
    * 函数描述	:  检查配置文件和内存中的表空间是否一致
    * 输入		:  
    * 输出		:  
    * 返回值	:  0- 成功; 非0- 失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int CheckTablespace(TMdbShmDSN* pMdbShmDSN);

    /******************************************************************************
    * 函数名称	:  CheckTables
    * 函数描述	:  检查配置文件和内存中的表是否一致
    * 输入		:  
    * 输出		:  
    * 返回值	:  0- 成功; 非0- 失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int CheckTables(TMdbShmDSN* pMdbShmDSN);
    /******************************************************************************
    * 函数名称	:  LockFile()
    * 函数描述	:  缩住文件pszFile    
    * 输入		:  pszFile，待锁住的文件名
    * 输出		:  无
    * 返回值	:  成功返回0，失败返回其他值
    * 作者		:  li.shugang
    *******************************************************************************/
    int LockFile(const char* pszFile);	

    /******************************************************************************
    * 函数名称	:  UnLockFile()
    * 函数描述	:  释放作用在pszFile上的锁(不支持win32系统)
    * 输入		:  pszFile，待释放锁的文件名
    * 输出		:  无
    * 返回值	:  成功返回0，失败返回其他值
    * 作者		:  li.shugang
    *******************************************************************************/
    int UnLockFile(const char* pszFile);
	int CheckSysConfig(TMdbCfgDSN* pCfgDSN,TMdbShmDSN* pMdbShmDSN);//DSN配置校验
    /******************************************************************************
    * 函数名称	:  ReportState()
    * 函数描述	:  检查是否需要上报状态，如果需要，向mdbServer上报状态
    * 输入		:  
    * 输出		:  无
    * 返回值	:  成功返回0，失败返回其他值
    * 作者		:  Jiang.lili
    *******************************************************************************/
    int ReportState(EMdbRepState eState);
private:
    TMdbConfig * m_pConfig;
    char m_sDsn[MAX_NAME_LEN];  //要控制的内存数据库的DSN名称
    char m_sLockFile[MAX_FILE_NAME]; //需要锁住的文件名
    TMdbProcCtrl m_tProcCtrl;
    char sPeerIP[MAX_IP_LEN];
    int iPeerPort;
    bool m_bLoadFromDisk;//是否从磁盘加载
    TMdbShardBuckupCfgCtrl m_tSBCfgCtrl;
};

//}
#endif //__MINI_DATABASE_CTROL_H__


