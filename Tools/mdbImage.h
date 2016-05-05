/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbImage.h
*@Description： 内存数据库的镜像控制
*@Author:		jin.shaohua
*@Date：	    2012年11月27日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_IMAGE_H__
#define __MINI_DATABASE_IMAGE_H__
#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

    class TMdbImage
    {
    public:
        TMdbImage();
        ~TMdbImage();
        int Export(const char * sDsn,const char *sImageDir);//导出进行文件
        int Import(const char * sDsn,const char * sImageDir);//从镜像目录导入
    private:
        int FlushToImage(const char  sFileName[],char * pMemAddr,int iSize);//刷出镜像文件
        int CreateFromImage(const char  sFileName[],long lKey,long iSize,int &iShmID,char * & pAddr);//从镜像创建
        int AdjustTableSpace(TMdbDSN * pDsn);//调整表空间的shmid
        int FlushDsnMgrInfo(const char  sFileName[],TMdbDSN * pDsn );//刷出DSNmgr信息
        int ImportMgr(const char * sPrefix,TMdbDSN *  &pDsn); //加载管理区
        int ImportIndex(const char * sPrefix,TMdbDSN *  pDsn);//加载索引区
        int ImportData(const char * sPrefix,TMdbDSN *  pDsn);//加载数据区
        int ImportRepShm(const char * sPrefix,TMdbDSN *  pDsn);//加载同步区
    private:
        TMdbConfig *m_pConfig;
        TMdbShmDSN *m_pShmDSN;
    	SHAMEM_T m_iOldDataShmID[MAX_SHM_ID];
    };

//}

#endif //__MINI_DATABASE_IMAGE_H__



