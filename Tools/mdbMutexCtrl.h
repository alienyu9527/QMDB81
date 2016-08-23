/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbMutexCtrl.h		
*@Description： 内存数据库的锁控制程序
*@Author:		jiang.mingjun
*@Date：	    2010年05月05日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE__MUTEX_CTROL_H__
#define __MINI_DATABASE__MUTEX_CTROL_H__

#include "Helper/mdbConfig.h"

//namespace QuickMDB{


    class TMdbMutexCtrl
    {
    public:
        TMdbMutexCtrl();
        ~TMdbMutexCtrl();
    public:
        int Init(const char* pszDsn);     //初始化
        int ReNewAllMutex();   //创建某个内存数据库    
        int RenewDsnMutex();
        int RenewSeqMutex(const char * sSeqname = NULL);
        int RenewDataMutex();
        int RenewTableMutex(const char * sTablename = NULL);
        int RenewPageMutex();
        int RenewTablespaceMutex(const char * sTSname = NULL);
        #if 0
        int RenewOraMutex();
        int RenewRepMutex();
        int RenewCaptureMutex();
        #endif
        
        int RenewIndexMutex();
        int RenewSyncAreaMutex(int iType);
		int RenewShardBakBufAreaMutex(int iType);
    private:
        int RenewOneMutex(TMutex & tMutex);
        TMdbShmDSN * m_pShmDsn;
    };

//}

#endif //__MINI_DATABASE_CTROL_H__


