/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbInfo.h       
*@Description： 负责打印内存数据库的各种信息
*@Author:       li.shugang
*@Date：        2009年03月10日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_INFORMATION_H__
#define __MINI_DATABASE_INFORMATION_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{


    class TMdbInfo
    {
    public:
        TMdbInfo(bool bDetail);
        TMdbInfo();
        ~TMdbInfo();
        
        /******************************************************************************
        * 函数名称  :  Connect()
        * 函数描述  :  链接某个DSN，但是不在管理区注册任何信息    
        * 输入      :  pszDSN, 锁管理区所属的DSN 
        * 输出      :  无
        * 返回值    :  成功返回0, 失败返回-1
        * 作者      :  li.shugang
        *******************************************************************************/
        int Connect(const char* pszDSN);   
        

        /******************************************************************************
        * 函数名称  :  PrintLink()
        * 函数描述  :  打印链接信息    
        * 输入      :  iFlag, 1-打印本地链接, 2-打印远程链接, 0-打印所有链接
        * 输出      :  无
        * 返回值    :  无
        * 作者      :  li.shugang
        *******************************************************************************/
        void PrintLink(int iFlag=0, int iPid=0);
        

        /******************************************************************************
        * 函数名称  :  PrintProc()
        * 函数描述  :  打印进程信息    
        * 输入      :  pszProc, 进程名称
        * 输出      :  无
        * 返回值    :  无
        * 作者      :  li.shugang
        *******************************************************************************/
        void PrintProc(const char* pszProc=NULL);
        

        /******************************************************************************
        * 函数名称  :  PrintMem()
        * 函数描述  :  打印内存块信息    
        * 输入      :  pszShmID, Share-Memory-ID
        * 输出      :  无
        * 返回值    :  无
        * 作者      :  li.shugang
        *******************************************************************************/
        int PrintMem(const char* pszShmID=NULL);
        

        /******************************************************************************
        * 函数名称  :  PrintProc()
        * 函数描述  :  打印表空间信息    
        * 输入      :  pszTableSpace, 表空间名称
        * 输出      :  无
        * 返回值    :  无
        * 作者      :  li.shugang
        *******************************************************************************/
        void PrintTableSpace(const char* pszTableSpace=NULL);
        

        /******************************************************************************
        * 函数名称  :  PrintTable()
        * 函数描述  :  打印表信息    
        * 输入      :  pszTable, 表名称
        * 输出      :  无
        * 返回值    :  无
        * 作者      :  li.shugang
        *******************************************************************************/
        void PrintTable(const char* pszTable=NULL);
        
        /******************************************************************************
        * 函数名称  :  PrintSQL()
        * 函数描述  :  打印系统SQL信息    
        * 输入      :  iPos, SQL位置，如果为-1表示打印全部
        * 输出      :  无
        * 返回值    :  无
        * 作者      :  li.shugang
        *******************************************************************************/
        void PrintSQL(int iPos=-1);
        
        void PrintDSN(const char* pszDSN);
        void PrintSeq(const char* pszSeq=NULL);
        void PrintJob(const char * sJobName = NULL);//打印job列表
        int SetRouterToCapture(const char * sRouter);//设置要捕获的路由
    	/******************************************************************************
    	* 函数名称	: PrintUser 
    	* 函数描述	: 打印用户列表(主要是为了查询用户密码明文)
    	* 输入		:  sUserName 用户名或all
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  cao.peng
    	*******************************************************************************/
    	void PrintUser(const char * sUserName);
    	int EstimateTableCostMemory(TMdbTable * pTable);//预估表内存消耗
    	void PrintUsageOfLock();//打印锁使用情况
        void PrintRoutingRep();//打印分片备份路由信息
        void PrintNotLoadFromDBInfo();//控制表从数据库加载的附加配置信息
		void PrintVarcharPageList();
	private:
        TMdbConfig *m_pConfig;
        
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn;     
        TMdbTable  *m_pMdbTable;
        TMdbTableSpace *m_pMdbTableSpace;
        TMdbProc       *m_pMdbProc;
        TMdbLocalLink  *m_pMdbLocalLink;
        //TMdbRemoteLink *m_pMdbRemoteLink;
        bool m_bMore;

		
		TShmAlloc m_tMgrShmAlloc;//共享内存分配器
		char *  m_pMgrAddr;

        
    };
//}
//数据库的内存信息
class TMdbSizeInfo
{
public:
    class TResourceSize
    {//资源信息
    public:
        double  dTotalSize;
        double  dUsedSize;
        char    sDataType[32];
    public:
        void Clear()
        {
            dTotalSize      = 0.0;
            dUsedSize       = 0.0;
            sDataType[0]    = 0;
        }
        void Print()
        {
            printf("%-32s %15.1fM %15.1fM %15.1fM\n", 
                sDataType, 
                dTotalSize, 
                dUsedSize,
                dTotalSize - dUsedSize);
        }
    };
    class TTableSize
    {//表信息
    public:
        double  dDataSize;//数据
        double  dIndexSize;//索引
        int     iTotalCount;//记录总数
        int     iUsedCount;//已使用
        char    sTableName[MAX_NAME_LEN];
    public:
        void Clear()
        {
            dDataSize       = 0.0;
            dIndexSize      = 0.0;
            iTotalCount     = 0;
            iUsedCount      = 0;
            sTableName[0]   = 0;
        }
        void Print()
        {
            printf("%-32s %15.1fM %15.1fM %16d %16d\n",
                sTableName,
                dDataSize,
                dIndexSize,
                iTotalCount,
                iUsedCount);
        }
    };
public:
    TMdbSizeInfo();
    ~TMdbSizeInfo();
    
    int Init(const char * sDsn);
    void PrintResourceInfo(bool bDetail);//打印资源的内存占用
    void PrintTableInfo(const char *sTableName,int iCount);//打印qmdb表的内存占用
private:
    void GetMgrSize(TResourceSize & data);
    void GetDataBlockSize(TResourceSize & data);
    void GetIndexBlockSize(TResourceSize & data);
    void GetHashIndexSize(TResourceSize & data);
    void GetMHashIndexSize(TResourceSize & data);
    void GetMHashOtherSize(TResourceSize & data);
    void GetVarcharBlockSize(TResourceSize & data);
    void GetSyncSize(TResourceSize & data);
	void GetSBSyncSize(int iHostID, TResourceSize &data);
    void GetOneTableSize(TMdbTable * pTable,TTableSize &tTableSize);
    
private:
    TMdbConfig      *m_pConfig;
    TMdbShmDSN      *m_pShmDSN;
    TMdbDSN         *m_pDsn; 
	TMdbShmRepMgr   *m_pShmMgr;
};


#endif //__MINI_DATABASE_INFORMATION_H__



