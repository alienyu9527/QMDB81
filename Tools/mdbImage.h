/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbImage.h
*@Description�� �ڴ����ݿ�ľ������
*@Author:		jin.shaohua
*@Date��	    2012��11��27��
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
        int Export(const char * sDsn,const char *sImageDir);//���������ļ�
        int Import(const char * sDsn,const char * sImageDir);//�Ӿ���Ŀ¼����
    private:
        int FlushToImage(const char  sFileName[],char * pMemAddr,int iSize);//ˢ�������ļ�
        int CreateFromImage(const char  sFileName[],long lKey,long iSize,int &iShmID,char * & pAddr);//�Ӿ��񴴽�
        int AdjustTableSpace(TMdbDSN * pDsn);//������ռ��shmid
        int FlushDsnMgrInfo(const char  sFileName[],TMdbDSN * pDsn );//ˢ��DSNmgr��Ϣ
        int ImportMgr(const char * sPrefix,TMdbDSN *  &pDsn); //���ع�����
        int ImportIndex(const char * sPrefix,TMdbDSN *  pDsn);//����������
        int ImportData(const char * sPrefix,TMdbDSN *  pDsn);//����������
        int ImportRepShm(const char * sPrefix,TMdbDSN *  pDsn);//����ͬ����
    private:
        TMdbConfig *m_pConfig;
        TMdbShmDSN *m_pShmDSN;
    	SHAMEM_T m_iOldDataShmID[MAX_SHM_ID];
    };

//}

#endif //__MINI_DATABASE_IMAGE_H__



